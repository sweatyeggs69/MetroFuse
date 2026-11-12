#pragma once
// Redux Mode (Settings > Cheats/Mods): optional quality-of-life tweaks inspired by ShadowOne333's
// Metroid Fusion Redux and the community hacks it collects. Redux itself is a patch for the
// Japanese release; these are MetroFuse's own versions for the USA ROM, written against the
// Metroid Fusion decompilation. Nothing from the Redux patch files is used.
//
// Two kinds of tweak:
//  - ROM tweaks change a few bytes of the game's code in the emulator's copy of the ROM (the file on
//    disk is never touched). Each patch lists the bytes it expects to replace, so a tweak that does
//    not match the ROM is reported unavailable instead of corrupting it. sync() applies and reverts
//    them between frames, so they can be switched while playing.
//  - Host tweaks adjust IWRAM between frames, like the cheats do.
//
// Settings (metrofuse::Config): redux.<id> for every tweak below (all off by default). All tweaks
// are off while RetroAchievements hardcore mode is on.
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

#include "app/i18n.h"

namespace metrofuse::redux {

struct Patch {
    std::uint32_t address;  // GBA address in the ROM area (0x08000000 + file offset)
    std::uint8_t size;
    std::array<std::uint8_t, 8> original;
    std::array<std::uint8_t, 8> replacement;
};

enum class Kind { Rom, Host };

struct Tweak {
    const char* id;     // config key suffix
    const char* label;  // shown in the menu
    const char* help;   // one line under the checkbox
    Kind kind;
    std::span<const Patch> patches;  // Kind::Rom only
};

namespace detail {
// USA (AMTE, rev 0) addresses. See the comments for what each one changes.

// HudDrawEnergy: the "energy below 30" test that turns the alarm on becomes a jump to the branch
// that turns it off.
inline constexpr Patch kLowHealthAlarm[] = {
    {0x08071DA2, 2, {0x1D, 0x2E}, {0x15, 0xE0}},  // cmp r6,#29 -> b 0x08071DD0
};
// SamusCheckShooting: drop the "has Charge Beam" test so holding B charges the starting beam too.
inline constexpr Patch kChargeNormalBeam[] = {
    {0x08005CF0, 2, {0x1A, 0xD0}, {0xC0, 0x46}},  // beq (no Charge Beam) -> nop
};
// Navigation and message text: always take the "A held" fast path.
inline constexpr Patch kFastText[] = {
    {0x08079C9C, 2, {0x06, 0xD0}, {0xC0, 0x46}},
    {0x0807A216, 2, {0x03, 0xD0}, {0xC0, 0x46}},
};
// PauseScreenMapDrawSpecialOam: the "game completed" test for the item counts and timer on the map
// screen always passes.
inline constexpr Patch kPauseListTimer[] = {
    {0x08077F60, 2, {0x08, 0x56}, {0x01, 0x20}},  // ldrsb r0,[r1,r0] -> movs r0,#1
};
// InGameTimerUpdate: a second is 60 frames (the game counts 64).
inline constexpr Patch kAccurateClock[] = {
    {0x08068678, 2, {0x3F, 0x28}, {0x3B, 0x28}},  // cmp r0,#63 -> cmp r0,#59
};
// Samus riding an elevator: 2 pixels per frame instead of 1, in both directions.
inline constexpr Patch kFastElevators[] = {
    {0x08009BFC, 2, {0x04, 0x20}, {0x08, 0x20}},                        // movs r0,#4 -> #8
    {0x08009C0C, 4, {0xFC, 0xFF, 0x00, 0x00}, {0xF8, 0xFF, 0x00, 0x00}},  // -4 -> -8
};
// ColorFadingUpdate_DoorTransition: twice the scroll speed. Horizontal already clamps to the
// target. Vertical moves 2 units only while that does not pass the target, and finishes within one
// unit (a quarter pixel) of it, so an odd distance can never make it overshoot back and forth; the
// game copies the final background position at the end of the transition anyway.
inline constexpr Patch kFastDoors[] = {
    // up:   r0 = cur + 2; if (r0 > target) goto down; (nop); store r0
    {0x0806EBAC, 8, {0x08, 0x1C, 0x82, 0x42, 0x06, 0xD9, 0x48, 0x1C}, {0x88, 0x1C, 0x90, 0x42, 0x06, 0xDC, 0xC0, 0x46}},
    // down: r0 = cur - 2; if (r0 < target) goto done; store r0
    {0x0806EBC0, 6, {0x82, 0x42, 0x02, 0xD2, 0x48, 0x1E}, {0x88, 0x1E, 0x90, 0x42, 0x01, 0xDB}},
    {0x0806EBE6, 2, {0x06, 0x30}, {0x0C, 0x30}},  // horizontal += 6 -> 12
    {0x0806EC04, 2, {0x06, 0x38}, {0x0C, 0x38}},  // horizontal -= 6 -> 12
};
}  // namespace detail

inline constexpr Tweak kTweaks[] = {
    {"skip_intro", N_("Skip the opening with START"), N_("Press START during the new-file intro to jump to the start of the game."), Kind::Host, {}},
    {"fast_text", N_("Fast text"), N_("Messages and Adam's briefings scroll at full speed without holding A."), Kind::Rom, detail::kFastText},
    {"fast_doors", N_("Faster door transitions"), N_("Rooms scroll in twice as fast when you go through a door."), Kind::Rom, detail::kFastDoors},
    {"fast_elevators", N_("Faster elevators"), N_("Elevators move twice as fast."), Kind::Rom, detail::kFastElevators},
    {"pause_list_timer", N_("Item counts and timer on the map"), N_("The pause map shows collected tanks and play time on a first playthrough."), Kind::Rom, detail::kPauseListTimer},
    {"accurate_clock", N_("Accurate in-game clock"), N_("A second is 60 frames, so the clock matches real time."), Kind::Rom, detail::kAccurateClock},
    {"charge_normal_beam", N_("Chargeable starting beam"), N_("Hold B to charge the beam before you find the Charge Beam."), Kind::Rom, detail::kChargeNormalBeam},
    {"morph_idle", N_("Morph Ball rests when still"), N_("The Morph Ball only rolls while it is moving."), Kind::Host, {}},
    {"no_low_health_alarm", N_("No low health alarm"), N_("Turns off the beeping when energy is low."), Kind::Rom, detail::kLowHealthAlarm},
};
inline constexpr std::size_t kTweakCount = sizeof(kTweaks) / sizeof(kTweaks[0]);

// Which tweaks are on, indexed like kTweaks.
using Flags = std::array<bool, kTweakCount>;

inline constexpr std::size_t index_of(const char* id) {
    for (std::size_t i = 0; i < kTweakCount; ++i) {
        const char* a = kTweaks[i].id;
        const char* b = id;
        while (*a && *a == *b) ++a, ++b;
        if (*a == *b) return i;
    }
    return kTweakCount;
}

namespace detail {
inline bool matches(std::span<const std::uint8_t> rom, const Patch& p, const std::array<std::uint8_t, 8>& bytes) {
    const std::size_t off = p.address - 0x08000000u;
    if (off + p.size > rom.size()) return false;
    for (std::size_t i = 0; i < p.size; ++i)
        if (rom[off + i] != bytes[i]) return false;
    return true;
}
}  // namespace detail

// True when every patch of a ROM tweak finds the bytes it expects in the unmodified ROM. Host
// tweaks are always available.
inline bool available(std::span<const std::uint8_t> clean_rom, const Tweak& t) {
    for (const Patch& p : t.patches)
        if (!detail::matches(clean_rom, p, p.original)) return false;
    return true;
}

// Applies the ROM tweaks in `want` and reverts the others, in the emulator's ROM. `ok` is
// available() for each tweak, worked out once from the unmodified ROM; unavailable ones are never
// written. Returns true when anything changed. Call between frames.
inline bool sync(std::span<std::uint8_t> rom, const Flags& want, const Flags& ok) {
    bool changed = false;
    for (std::size_t i = 0; i < kTweakCount; ++i) {
        const Tweak& t = kTweaks[i];
        if (t.kind != Kind::Rom || !ok[i]) continue;
        for (const Patch& p : t.patches) {
            const auto& bytes = want[i] ? p.replacement : p.original;
            if (detail::matches(rom, p, bytes)) continue;
            const std::size_t off = p.address - 0x08000000u;
            for (std::size_t k = 0; k < p.size; ++k) rom[off + k] = bytes[k];
            changed = true;
        }
    }
    return changed;
}

// IWRAM addresses (offsets from 0x03000000) used by the host tweaks, from the decompilation's
// symbol map.
inline constexpr std::size_t kMainGameMode = 0x0BDE;  // u16, 7 = file select / new-file intro
inline constexpr std::size_t kSubGameMode1 = 0x0BE0;  // u16, intro scene 0..12
inline constexpr std::size_t kSubGameMode2 = 0x0BE2;  // u16, 1 = new-file intro
inline constexpr std::size_t kCutsceneTimer2 = 0x1484 + 0x20E;  // gNonGameplayRam.cutscene.unk_20E
inline constexpr std::size_t kSamusPose = 0x1244 + 0x01;        // gSamusData.pose
inline constexpr std::size_t kSamusAnimCounter = 0x1244 + 0x21;  // gSamusData.animationDurationCounter
inline constexpr std::size_t kSamusAnimFrame = 0x1244 + 0x22;    // gSamusData.currentAnimationFrame
inline constexpr std::uint8_t kPoseMorphBall = 0x0D;             // SPOSE_MORPH_BALL (still; rolling is 0x0E)
inline constexpr std::uint16_t kKeyStart = 1 << 3;

// Host tweaks. Call before_frame() just before running a frame and after_frame() right after it,
// with the keys held for that frame.
class Host {
public:
    void before_frame(std::span<std::uint8_t> iwram, const Flags& want, std::uint16_t keys) {
        if (iwram.size() < 0x8000) return;
        const bool start_pressed = (keys & kKeyStart) && !(prev_keys_ & kKeyStart);
        prev_keys_ = keys;
        if (want[index_of("skip_intro")] && start_pressed && get16(iwram, kMainGameMode) == 7 &&
            get16(iwram, kSubGameMode2) == 1) {
            const std::uint16_t scene = get16(iwram, kSubGameMode1);
            // Scenes 1..11 are the cutscene; 12 fades out and writes the new file, then the game
            // moves on to the map screen exactly as when the intro ends by itself.
            if (scene >= 1 && scene <= 11) {
                set16(iwram, kSubGameMode1, 12);
                set16(iwram, kCutsceneTimer2, 0);
            }
        }
        pose_ = iwram[kSamusPose];
        frame_ = iwram[kSamusAnimFrame];
    }

    void after_frame(std::span<std::uint8_t> iwram, const Flags& want) {
        if (iwram.size() < 0x8000) return;
        // A still Morph Ball keeps its current frame instead of rolling in place. Only while the
        // ball was already still before this frame, so the frame it stops on is kept.
        if (want[index_of("morph_idle")] && pose_ == kPoseMorphBall && iwram[kSamusPose] == kPoseMorphBall) {
            iwram[kSamusAnimFrame] = frame_;
            iwram[kSamusAnimCounter] = 0;
        }
    }

private:
    static std::uint16_t get16(std::span<const std::uint8_t> m, std::size_t at) {
        return std::uint16_t(m[at] | (m[at + 1] << 8));
    }
    static void set16(std::span<std::uint8_t> m, std::size_t at, std::uint16_t v) {
        m[at] = std::uint8_t(v);
        m[at + 1] = std::uint8_t(v >> 8);
    }
    std::uint16_t prev_keys_ = 0;
    std::uint8_t pose_ = 0, frame_ = 0;
};

}  // namespace metrofuse::redux
