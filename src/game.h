#pragma once
// The one place that describes the game MetroFuse runs. Everything game-specific that the rest of
// the code needs (names shown to the user, ROM fingerprint, save layout) lives here, so the
// launcher, packaging and tests do not hard-code it.
//
// Target: Metroid Fusion, USA release (AGB-AMTE), revision 0.
//
// Values marked TODO(verify) must be checked against your own dump before the first release.
// Nothing here is game data: these are only identifiers and a fingerprint of the user's own ROM.

#include <cstddef>
#include <string_view>

namespace metrofuse::game {

// Shown in the launcher, dialogs and error messages.
inline constexpr const char* kTitle = "Metroid Fusion";
inline constexpr const char* kRegion = "USA";
inline constexpr const char* kTitleWithRegion = "Metroid Fusion (USA)";
// Used for shortcuts, default export names, etc. (no characters that are invalid in file names).
inline constexpr const char* kFileSafeTitle = "Metroid Fusion";

// GBA cartridge header of the supported dump.
inline constexpr const char* kHeaderTitle = "METROID4USA";  // 0xA0, 12 bytes
inline constexpr const char* kGameCode = "AMTE";            // 0xAC, 4 bytes
inline constexpr unsigned kRevision = 0;                    // 0xBC
inline constexpr std::size_t kRomSize = 8 * 1024 * 1024;

// SHA-1 of the known-good dump (lowercase hex), checked against the maintainer's own cartridge
// dump on 2026-11-10 (MD5 af5040fc0f579800151ee2a683e2e5b5, CRC32 6C75479C). If this is ever
// emptied, a ROM is accepted on a header match instead and the launcher labels it
// "header match (fingerprint not pinned yet)".
inline constexpr const char* kExpectedSha1 = "ca33f4348c2c05dd330d37b97e2c5a69531dfe87";

// Battery save: 32 KiB SRAM (the ROM carries the SRAM_V112 library string).
inline constexpr std::size_t kSaveSize = 32 * 1024;
// Bytes the game writes at SRAM offset 0 on first boot, used to reject saves from other games.
// Empty disables the check. Checked on 2026-11-10 against a fresh first-boot save and a save made
// at an in-game Save Room: both start with these 32 bytes (a fixed tag, "01", then the game's
// "MetroidAGB-EUVerE004" save-format string). It holds a zero byte, so the length is explicit.
inline constexpr std::string_view kSaveHeader{"\xb4\xc8\x9b\xa1" "K7d^" "01" "\x00\x02" "MetroidAGB-EUVerE004", 32};

// Samus's equipment (energy, missiles, power bombs) in IWRAM, used by the cheats menu. Layout
// from the community disassembly (struct Equipment at gEquipment):
//   +0 u16 current energy   +2 u16 max energy
//   +4 u16 current missiles +6 u16 max missiles
//   +8 u8 current power bombs  +9 u8 max power bombs
inline constexpr unsigned kEquipmentAddress = 0x03001310;
// +0xC u8 suit/misc ability flags; this bit is set once the Morph Ball is collected.
inline constexpr unsigned kSuitMorphBall = 1u << 6;

// Samus's state (struct SamusData at gSamusData in the community disassembly). +1 u8 pose.
inline constexpr unsigned kSamusDataAddress = 0x03001244;
// Pose while crouched on the ground; a new Down press here morphs if the Morph Ball is owned.
inline constexpr unsigned kPoseCrouching = 7;

}  // namespace metrofuse::game
