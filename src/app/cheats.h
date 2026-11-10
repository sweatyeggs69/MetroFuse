#pragma once
// Cheats menu toggles (F1 > Cheats). After every game frame the host calls apply() on the game's
// IWRAM, which tops the chosen supplies back up to their current maximum. Nothing is written while
// a maximum is still 0 (before a file is loaded). Only the counts change, never the weapon flags,
// so a cheat never unlocks missiles or power bombs early. Off while RetroAchievements hardcore
// mode is on.
//
// Settings (metrofuse::Config): cheats.health, cheats.missiles, cheats.power_bombs (all off by default).
#include <cstddef>
#include <cstdint>
#include <span>

#include "game.h"

namespace metrofuse::cheats {

struct Flags {
    bool health = false;
    bool missiles = false;
    bool power_bombs = false;
    bool any() const { return health || missiles || power_bombs; }
};

namespace detail {
inline std::uint16_t get16(std::span<const std::uint8_t> m, std::size_t at) {
    return std::uint16_t(m[at] | (m[at + 1] << 8));
}
inline void set16(std::span<std::uint8_t> m, std::size_t at, std::uint16_t v) {
    m[at] = std::uint8_t(v);
    m[at + 1] = std::uint8_t(v >> 8);
}
}  // namespace detail

// Refills the enabled supplies in iwram (the 32 KiB IWRAM, starting at 0x03000000). Returns true
// when anything changed.
inline bool apply(std::span<std::uint8_t> iwram, const Flags& f) {
    const std::size_t base = game::kEquipmentAddress - 0x03000000u;
    if (!f.any() || iwram.size() < base + 10) return false;
    bool changed = false;
    auto refill16 = [&](std::size_t cur, std::size_t max) {
        const std::uint16_t m = detail::get16(iwram, base + max);
        if (m != 0 && detail::get16(iwram, base + cur) != m) {
            detail::set16(iwram, base + cur, m);
            changed = true;
        }
    };
    if (f.health) refill16(0, 2);
    if (f.missiles) refill16(4, 6);
    if (f.power_bombs) {
        const std::uint8_t m = iwram[base + 9];
        if (m != 0 && iwram[base + 8] != m) {
            iwram[base + 8] = m;
            changed = true;
        }
    }
    return changed;
}

}  // namespace metrofuse::cheats
