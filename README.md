<p align="center">
  <img src="assets/icon/metrofuse-icon.png" alt="MetroFuse app icon" width="128">
</p>

<p align="center">
  <img src="assets/logo/metrofuse-logo.png" alt="MetroFuse" width="520">
</p>

<p align="center">
  Play <b>Metroid Fusion</b> natively on macOS, Windows and Linux.
</p>

<p align="center">
  <a href="https://github.com/sweatyeggs69/MetroFuse/actions/workflows/build.yml"><img alt="Build" src="https://github.com/sweatyeggs69/MetroFuse/actions/workflows/build.yml/badge.svg"></a>
  <a href="LICENSE"><img alt="License: MIT" src="https://img.shields.io/badge/license-MIT-blue"></a>
  <img alt="Platforms" src="https://img.shields.io/badge/platforms-macOS%20%7C%20Windows%20%7C%20Linux-5b8def">
</p>

> [!IMPORTANT]
> MetroFuse does not include the game. You need your own legally dumped ROM of
> **Metroid Fusion (USA)**, which MetroFuse checks before playing.
> MetroFuse is an unofficial fan project. It is not affiliated with, endorsed or sponsored by
> Nintendo. Metroid and all related names are trademarks of their respective owners.

> [!NOTE]
> **Early development.** MetroFuse is built on the same runtime, launcher, platform layer and
> tooling as [KNID](https://github.com/sweatyeggs69/KNID). The game-specific parts are still being
> filled in; see [Porting status](#porting-status).

## Getting started

1. **Build or download** MetroFuse for your system and open it.
2. **Choose your ROM.** Click **Choose ROM...** and pick your dump. MetroFuse only remembers where
   the file is; it never copies, uploads or changes it.
3. **Play.** Press **Play**. To start the game directly next time, add a shortcut in **Settings > Game**.

## Features

Inherited from the KNID platform layer:

- **Native on macOS, Windows and Linux**, with smooth, tear-free video and sound.
- **Themed launcher** with **Continue** to pick up where you left off.
- **One settings menu (F1)** for display (scaling, widescreen, screen effects) and controls.
- **Controllers and remapping** for keyboard and gamepads.
- **Save states**: ten slots plus Auto, with thumbnails, quick save/load and optional auto-load.
- **Cheats** (F1 > Cheats/Mods): unlimited health, missiles and power bombs. Off in RetroAchievements hardcore mode.
- **Redux Mode** (F1 > Cheats/Mods): optional quality-of-life tweaks, switched on together or one at a time: skip the
  opening with START, fast text, faster doors and elevators, item counts and timer on the map, an accurate clock, a
  chargeable starting beam, a Morph Ball that rests when still, and no low health alarm. Inspired by
  [Metroid Fusion Redux](https://github.com/ShadowOne333/Metroid-Fusion-Redux) by ShadowOne333 and the community hacks
  it collects; these are MetroFuse's own versions for the USA game, applied in memory (your ROM file is never changed).
  Off in RetroAchievements hardcore mode.
- **Import saves** from RetroArch, mGBA, VBA-M and others.
- **RetroAchievements**, optional.
- **Automatic updates** from this repository's releases, with your OK, without touching your saves.
- **Optional BIOS** with the original boot animation (not required).

## Porting status

Everything game-specific is described in one file, [`src/game.h`](src/game.h). Items marked
`TODO(verify)` there must be checked against a real dump before the first release.

| Area | Status |
| --- | --- |
| Build system, CI, packaging (DMG, single-file EXE, AppImage) | Done (from KNID, renamed) |
| GBA runtime, launcher, options, save states, updater, achievements | Done (game-agnostic) |
| ROM check | Done: SHA-1 pinned to the USA dump (`AMTE` rev 0, 8 MiB) |
| Battery saves | Done: 32 KiB SRAM, with the save signature (`game::kSaveHeader`) checked on import so saves from other games are rejected |
| Widescreen | Done for 16:10 and 16:9: in gameplay the extra columns are rebuilt from the room layout in RAM (`src/gba/wide_level.cpp`); title, cutscenes and menus keep black sides, as does anything past the room edge. The HUD stays in the original 240-pixel view. Wider shapes are not offered because the game stops drawing enemies and other objects about 32 pixels past the original view |
| App icon and theme | Done: original MF icon and logo. The launcher and F1 menu use the shared GBA-port theme (`src/app/theme.cpp`, the same in every port, styled after Bookie); MetroFuse's colors are in `src/app/theme_colors.cpp` |
| RecompLauncher metadata | Done (`recomplauncher.json`) |
| Save states across BIOS settings | Done: states load after switching between the real and built-in BIOS. Fusion waits inside the BIOS Halt call almost every frame, and the converter backs that call out and runs it again. A state saved on a frame that ends mid-way through a BIOS decompression (about 1 frame in 100, during room loads) is refused with a message; switch the BIOS back to load it |

## Controls

Defaults; change them in Settings > Controls.

| Action | Keyboard | Gamepad |
| --- | --- | --- |
| D-pad | Arrow keys | D-pad or left stick |
| A / B | X / Z | South / East (also North / West) |
| L / R | A / S | Shoulders or triggers |
| Start / Select | Enter / Backspace | Start / Back |
| Settings menu | F1 or Esc | |
| Quick save / load | Page Up / Page Down | Back + R / Back + L |
| Save to new slot | F9 | |
| Previous / next slot | F6 / F7 | Back + D-pad left / right |
| Fullscreen | F11 | |

Settings > Controls > Extra controls adds optional buttons, all unbound until you set them:

- **Missile toggle**: press once to arm missiles, press again to go back to the beam (instead of
  holding R). In morph ball it arms power bombs, the same as holding R.
- **Aim diagonally up / down**: hold to aim, instead of holding L (and L + Down).
- **Right stick aims diagonally**: push the right stick up or down to aim.

START already opens the map in Fusion, so there is no separate map button.

Any button a controller reports can be bound, including paddles, back buttons, extra buttons,
D-pads on arcade sticks, and mouse buttons 4 and 5. Controllers SDL has no layout for are read
as raw joysticks, so their buttons show up by number. Buttons remapped by Steam Input or another
remapper work as whatever key or button they send.

Quick save and quick load use the Auto slot until you pick another slot (F6 / F7, or click a slot in
the States tab). Closing the game also saves to the Auto slot.

## FAQ

**Which version of the game is supported?**
Metroid Fusion, USA release (game code `AMTE`, revision 0).

**Do I need a GBA BIOS?**
No. MetroFuse has a built-in replacement. If you own a BIOS dump, Settings > Game > BIOS shows the
original boot animation.

**Where are my saves?**
In your user folder, never inside the app: `~/Library/Application Support/MetroFuse/MetroFuse/`
(macOS), `%APPDATA%\MetroFuse\MetroFuse\` (Windows) or `~/.local/share/MetroFuse/MetroFuse/`
(Linux).

<details>
<summary><b>Building from source</b></summary>

You need CMake 3.24+, a C++20 compiler and Git. SDL3, Dear ImGui and rcheevos are fetched
automatically.

```sh
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DMETROFUSE_VENDOR_SDL=ON
cmake --build build
ctest --test-dir build --output-on-failure
./build/metrofuse
```

- **macOS app and DMG:** add `-DMETROFUSE_MACOS_BUNDLE=ON "-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64"`
  and build the `dmg` target.
- **Windows:** Visual Studio 2022; `vcpkg install zlib:x64-windows-static` and pass the vcpkg
  toolchain with `-DVCPKG_TARGET_TRIPLET=x64-windows-static -DMETROFUSE_PACKAGE=ON`, then build
  `package_exe`.
- **Linux:** install the SDL build dependencies (X11/Wayland, ALSA/PulseAudio, `zlib1g-dev`); see
  `.github/workflows/build.yml` for the full list.
- **Analysis tools:** `metrofuse_disasm` (needs Capstone) traces the ROM into `generated/`, which
  is gitignored. Its output is derived from the game and must never be committed.

Tests that need the game look for `Metroid Fusion.gba` in the source folder (or `$METROFUSE_ROM`)
and skip themselves otherwise. Never commit ROMs, BIOS files, saves, states or screenshots of the
game.

</details>

<details>
<summary><b>For maintainers</b></summary>

- **Releases:** bump `VERSION` in `project(metrofuse ...)` in `CMakeLists.txt`, then push a matching
  tag like `v1.0.0`. GitHub Actions builds all three platforms and publishes the release. Keep the
  file names (`MetroFuse-<version>-macOS.dmg`, `MetroFuse-<version>-Windows-x64.exe`,
  `MetroFuse-<version>-Linux-x64.AppImage`, `SHA256SUMS.txt`); the in-app updater looks for them.

</details>

## Contributing

Bug reports and pull requests are welcome. Keep game content out of the repository: no ROMs, BIOS
files, saves, save states, extracted assets or screenshots of the game.

## License

[MIT](LICENSE). Bundled open-source components and fonts are listed in
[THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md).
