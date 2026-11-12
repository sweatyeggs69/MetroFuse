#pragma once
// Hold Down to morph (Controls > Hold Down to morph, off by default). Fusion morphs when Down is
// newly pressed while Samus is crouching. With this on, keeping Down held while crouched for the
// chosen time lets go of Down for one frame and presses it again, so the game sees that new press
// and morphs. Nothing in the game's memory is written: the host only changes the buttons it sends.
//
// It only fires when the game would morph on a real second press: Samus is crouching on the ground
// (not aiming down in midair), she has the Morph Ball, and L (diagonal aim) is not held. It fires
// once per hold, so letting go of Down and holding it again is needed for the next time.
//
// Settings (metrofuse::Config): input.hold_morph (bool), input.hold_morph_ms (hold time).
#include <cstddef>
#include <cstdint>
#include <span>

#include "game.h"

namespace metrofuse::hold_morph {

inline constexpr int kMinMs = 150;
inline constexpr int kMaxMs = 1000;
inline constexpr int kDefaultMs = 300;

// GBA KEYINPUT bit order (1 = pressed in the host's mask).
inline constexpr std::uint16_t kDown = 1u << 7;
inline constexpr std::uint16_t kL = 1u << 9;

inline int frames_for_ms(int ms) {
    if (ms < kMinMs) ms = kMinMs;
    if (ms > kMaxMs) ms = kMaxMs;
    return (ms * 60 + 500) / 1000;
}

// True when a fresh Down press would morph Samus right now, read from IWRAM (starting at 0x03000000).
inline bool can_morph(std::span<const std::uint8_t> iwram) {
    const std::size_t pose = game::kSamusDataAddress + 1 - 0x03000000u;
    const std::size_t suit = game::kEquipmentAddress + 0xC - 0x03000000u;
    if (iwram.size() <= pose || iwram.size() <= suit) return false;
    return iwram[pose] == game::kPoseCrouching && (iwram[suit] & game::kSuitMorphBall) != 0;
}

class Assist {
public:
    // Call once per game frame with the buttons about to be sent and IWRAM as the last frame left
    // it. Returns the buttons to send.
    std::uint16_t step(std::uint16_t keys, std::span<const std::uint8_t> iwram, int hold_frames) {
        if (!(keys & kDown)) {
            held_ = 0;
            fired_ = false;
            return keys;
        }
        if (held_ < 0x7fff) ++held_;
        if (fired_ || (keys & kL) || held_ < hold_frames || !can_morph(iwram)) return keys;
        fired_ = true;  // Down is up for this frame; next frame it is pressed again
        return std::uint16_t(keys & ~kDown);
    }
    void reset() {
        held_ = 0;
        fired_ = false;
    }

private:
    int held_ = 0;  // frames Down has been held
    bool fired_ = false;
};

}  // namespace metrofuse::hold_morph
