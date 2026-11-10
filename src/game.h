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

// SHA-1 of the known-good dump (lowercase hex). TODO(verify): fill in from a verified No-Intro
// dump. While it is empty, a ROM is accepted when its header matches kGameCode/kRevision and its
// size is kRomSize, and the launcher labels it "header match (fingerprint not pinned yet)".
inline constexpr const char* kExpectedSha1 = "";

// Battery save. TODO(verify): Metroid Fusion is believed to use 32 KiB SRAM; confirm against the
// game's save type string in the ROM before relying on the importer's size checks.
inline constexpr std::size_t kSaveSize = 32 * 1024;
// Bytes the game writes at SRAM offset 0, used to reject saves from other games. Empty disables
// the check. TODO(verify): find Fusion's save signature and set it here.
inline constexpr const char* kSaveHeader = "";

}  // namespace metrofuse::game
