// Widescreen level reconstruction hook (game-specific).
//
// In widescreen the PPU draws extra columns left and right of the 240-pixel view. The GBA
// only keeps the visible part of a level in VRAM, so for those columns the port must rebuild
// tilemap entries from the level layout the game keeps in RAM. How that layout is stored is
// specific to each game, so this file is the one place that knows it.
//
// For Metroid Fusion this is not implemented yet: wide_level_detect() marks both layers
// invalid, so the PPU falls back to plain VRAM rendering for BG2/BG3 (the extra columns show
// whatever the VRAM ring buffer holds there). To implement it:
//   1. Find the routines that copy level tiles into the BG2/BG3 screen blocks (metrofuse_disasm
//      helps here) and the RAM addresses of the level layout, width and height they read.
//   2. In wide_level_detect(), read those addresses, sanity-check them, and find the absolute
//      camera position whose tiles match what is in VRAM (sample a grid of visible tiles).
//   3. In wide_level_entry(), return the tilemap entry for absolute tile (tx, ty).
// Only addresses and layout descriptions belong here; never copy game data into the repo.
#include "gba.h"

namespace metrofuse::gba {

u16 Gba::wide_ram16(u32 a) const {
    if ((a >> 24) == 0x02) return u16(ewram_[a & 0x3FFFF] | (ewram_[(a + 1) & 0x3FFFF] << 8));
    if ((a >> 24) == 0x03) return u16(iwram_[a & 0x7FFF] | (iwram_[(a + 1) & 0x7FFF] << 8));
    return 0;
}

u32 Gba::wide_ram32(u32 a) const { return u32(wide_ram16(a)) | (u32(wide_ram16(a + 2)) << 16); }

u16 Gba::wide_level_entry(int layer, s32 tx, s32 ty) const {
    (void)layer;
    (void)tx;
    (void)ty;
    return 0;  // TODO(metroid-fusion): rebuild the entry from the level layout in RAM.
}

void Gba::wide_level_detect() {
    for (auto& L : wide_level_) L.valid = false;
    // TODO(metroid-fusion): detect the active level layout and camera (see the notes above).
}

}  // namespace metrofuse::gba
