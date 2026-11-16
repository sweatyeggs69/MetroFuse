#pragma once
// Controller rumble (Settings > Controls > Rumble). The GBA has no rumble, so Watcher reads the
// game's RAM after every frame and turns what just happened into a Pulse; the host plays it on
// every connected controller (input::Manager::rumble), scaled by the strength setting.
//
// Events, all read from Metroid Fusion's RAM (addresses from the community disassembly; the
// equipment and Samus addresses are the ones the cheats and Auto Morph Ball already use):
//   - Samus takes damage: gEquipment (IWRAM 0x03001310) +0 u16 energy drops. Stronger for bigger
//     hits; heat, cold and acid that drain a little every few frames give a light buzz.
//   - Samus dies: energy reaches 0.
//   - the screen shakes: gScreenShakeY / gScreenShakeX (IWRAM 0x030000F0 / 0x030000F4,
//     struct { u8 timer; u8 delay; u8 intensity; u8 direction; }). The game shakes the screen for
//     explosions, heavy landings, bosses and the like, so the controller shakes with it.
//   - a power bomb goes off: gCurrentPowerBomb (IWRAM 0x03000110) +0 u8 state turns non-zero
//     (Samus's and the SA-X's).
//   - a missile is fired: +4 u16 missiles drops (a light tick).
// Nothing fires during the title screen demos (gDemoState, IWRAM 0x03001242, non-zero).
//
// Settings (metrofuse::Config): input.rumble (on by default), input.rumble_strength (10-100,
// default 75).
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <span>

#include "game.h"

namespace metrofuse::rumble {

inline constexpr bool kDefaultOn = true;
inline constexpr int kDefaultStrength = 75;
inline constexpr int kMinStrength = 10;
// Shown next to the Controller rumble toggle.
inline constexpr const char* kEventsHelp =
    "Shakes your controller when Samus is hurt, when the screen shakes (explosions, bosses, heavy "
    "landings), when a power bomb goes off and, lightly, when she fires a missile. Works with "
    "controllers that have rumble motors.";

// One rumble: motor strengths 0..1 (low = the big, slow motor, high = the small, fast one).
struct Pulse {
    float low = 0, high = 0;
    int ms = 0;
    explicit operator bool() const { return ms > 0 && (low > 0 || high > 0); }
    float strength() const { return std::max(low, high); }
};

// The stronger of two pulses; on a tie the longer one.
inline Pulse stronger(const Pulse& a, const Pulse& b) {
    if (!a) return b;
    if (!b) return a;
    if (a.strength() != b.strength()) return a.strength() > b.strength() ? a : b;
    return a.ms >= b.ms ? a : b;
}

// Decides which pulses reach the controllers: a new pulse replaces the one still playing only
// when it is at least as strong, so a missile tick never cuts an explosion short. Counts time in
// game frames (step once per emulated frame).
class Mixer {
public:
    // True when p should be played now.
    bool take(const Pulse& p) {
        if (!p) return false;
        if (left_ms_ > 0 && p.strength() < playing_.strength()) return false;
        playing_ = p;
        left_ms_ = p.ms;
        return true;
    }
    void step() { left_ms_ = std::max(0, left_ms_ - kFrameMs); }
    bool playing() const { return left_ms_ > 0; }
    void clear() { left_ms_ = 0; }

private:
    static constexpr int kFrameMs = 17;  // one GBA frame (16.74 ms)
    Pulse playing_;
    int left_ms_ = 0;
};

// Strength setting (percent) -> motor value for SDL (0..0xFFFF).
inline std::uint16_t motor(float level, int strength_percent) {
    const float v = std::clamp(level, 0.0f, 1.0f) * std::clamp(strength_percent, 0, 100) / 100.0f;
    return std::uint16_t(v * 65535.0f + 0.5f);
}

namespace detail {
inline std::uint16_t get16(std::span<const std::uint8_t> m, std::size_t at) {
    return std::uint16_t(m[at] | (m[at + 1] << 8));
}
}  // namespace detail

// ---- Metroid Fusion ------------------------------------------------------------------------

inline constexpr unsigned kScreenShakeY = 0x030000F0;
inline constexpr unsigned kScreenShakeX = 0x030000F4;
inline constexpr unsigned kPowerBomb = 0x03000110;
inline constexpr unsigned kDemoState = 0x03001242;

class Watcher {
public:
    // Forgets the last frame, so a jump in the numbers (a loaded state, rewind, a new game) is
    // not taken for an event.
    void reset() { primed_ = false; }

    // Call after each emulated frame with the 32 KiB IWRAM (0x03000000).
    Pulse step(std::span<const std::uint8_t> iwram) {
        using detail::get16;
        const std::size_t eq = game::kEquipmentAddress - 0x03000000u;
        if (iwram.size() < 0x8000) return {};
        Frame now;
        now.energy = get16(iwram, eq + 0);
        now.max_energy = get16(iwram, eq + 2);
        now.missiles = get16(iwram, eq + 4);
        now.max_missiles = get16(iwram, eq + 6);
        now.shake_y = iwram[kScreenShakeY - 0x03000000u];
        now.shake_x = iwram[kScreenShakeX - 0x03000000u];
        now.shake_power = std::max(iwram[kScreenShakeY - 0x03000000u + 2], iwram[kScreenShakeX - 0x03000000u + 2]);
        now.power_bomb = iwram[kPowerBomb - 0x03000000u];
        now.demo = iwram[kDemoState - 0x03000000u] != 0;

        const Frame before = last_;
        const bool primed = primed_;
        last_ = now;
        primed_ = true;
        if (!primed || now.demo || before.demo) return {};

        Pulse out;
        const bool same_file = now.max_energy == before.max_energy && now.max_energy != 0;
        if (same_file && now.energy < before.energy) {
            const int lost = before.energy - now.energy;
            if (now.energy == 0) out = stronger(out, {1.0f, 1.0f, 700});  // death
            else if (lost <= 2) out = stronger(out, {0.25f, 0.15f, 60});  // heat / cold / acid drain
            else {
                const float f = std::min(1.0f, 0.45f + lost / 60.0f);
                out = stronger(out, {f, f * 0.6f, 120 + std::min(lost, 60) * 2});
            }
        }
        // A shake starts (or is extended): the timer goes up.
        const int shake = std::max(now.shake_y > before.shake_y ? now.shake_y : 0,
                                   now.shake_x > before.shake_x ? now.shake_x : 0);
        if (shake > 0) {
            const float f = std::min(1.0f, 0.5f + now.shake_power * 0.1f);
            out = stronger(out, {f, f * 0.5f, std::clamp(shake * 17, 120, 900)});
        }
        if (now.power_bomb != 0 && before.power_bomb == 0) out = stronger(out, {1.0f, 0.7f, 900});
        if (now.max_missiles == before.max_missiles && now.missiles < before.missiles &&
            before.missiles - now.missiles <= 2)
            out = stronger(out, {0.0f, 0.35f, 70});
        return out;
    }

private:
    struct Frame {
        std::uint16_t energy = 0, max_energy = 0, missiles = 0, max_missiles = 0;
        std::uint8_t shake_y = 0, shake_x = 0, shake_power = 0, power_bomb = 0;
        bool demo = false;
    };
    Frame last_;
    bool primed_ = false;
};

}  // namespace metrofuse::rumble
