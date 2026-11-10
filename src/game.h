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
// Bytes the game writes at SRAM offset 0, used to reject saves from other games. Empty disables
// the check. TODO(verify): find Fusion's save signature and set it here.
inline constexpr const char* kSaveHeader = "";

// Samus's equipment (energy, missiles, power bombs) in IWRAM, used by the cheats menu. Layout
// from the community disassembly (struct Equipment at gEquipment):
//   +0 u16 current energy   +2 u16 max energy
//   +4 u16 current missiles +6 u16 max missiles
//   +8 u8 current power bombs  +9 u8 max power bombs
inline constexpr unsigned kEquipmentAddress = 0x03001310;

}  // namespace metrofuse::game
