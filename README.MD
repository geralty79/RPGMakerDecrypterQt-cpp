# RPG Maker Decrypter — Qt/C++ Edition

Cross-platform GUI rewrite of [RPG Maker Decrypter](https://github.com/uuksu/RPGMakerDecrypter)
in **C++17 with Qt 6**. Extract encrypted archives and files created with
**RPG Maker XP, VX, VX Ace, MV or MZ**, and reconstruct a best guess of the
original project so it can be opened in the editor again.

> This is a fork of the original .NET/C# project. The original C# CLI sources
> are kept for reference under [`legacy/`](legacy/) (`RPGMakerDecrypter.Cli`,
> `RPGMakerDecrypter.MVMZ`, `RPGMakerDecrypter.RGSSAD`,
> `RPGMakerDecrypter.Tests`) and can still be built with the .NET 8 SDK.

## Features

- **RPG Maker XP / VX / VX Ace archives** (`.rgssad`, `.rgss2a`, `.rgss3a`) —
  extracted with the original directory structure, including archives that
  have been renamed (the format version is read from the file header).
- **RPG Maker MV / MZ deployments** — encrypted assets (`.rpgmvo`, `.rpgmvp`,
  `.rpgmvm`, `.ogg_`, `.png_`, `.m4a_`) are decrypted in place using the
  encryption key from `System.json`. Windows, Linux and macOS `.app`
  deployments are supported.
- **Project reconstruction** — writes the project files that make the RPG
  Maker editors recognize the game again:
  - XP/VX/VX Ace: `Game.rxproj` / `Game.rvproj` / `Game.rvproj2` + `Game.ini`
  - MV: `Game.rpgproject`, MZ: `game.rmmzproject`
- **GUI comforts**: automatic RPG Maker version detection, output directory
  selection, overwrite protection, progress bar and log, and cancellation of
  long operations at any time.

## Requirements

- The GUI application is a native binary. Building it requires a C++17
  compiler, CMake 3.21+ and **Qt 6.4+** (`Widgets` and `Core` modules).
- The original C# CLI requires the .NET 8 SDK.

## Building (Qt/C++ GUI)

```bash
cmake -S RPGMakerDecrypter.Cpp -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On Debian/Ubuntu, install the toolchain with:

```bash
sudo apt install build-essential cmake qt6-base-dev
```

This produces the GUI application `build/RPGMakerDecrypter` and the
self-test binary `build/RPGMakerDecrypterTests` (run with `ctest`). See
[RPGMakerDecrypter.Cpp/README.md](RPGMakerDecrypter.Cpp/README.md) for
platform-specific notes and the roadmap in
[RPGMakerDecrypter.Cpp/PLAN.md](RPGMakerDecrypter.Cpp/PLAN.md).

## Using the GUI

1. Pick the input with **Browse File...** (an RGSSAD archive) or
   **Browse Folder...** (an MV/MZ deployment directory) — the RPG Maker
   version is detected automatically and shown.
2. Optionally choose an output directory. Empty means: archives extract next
   to the archive, MV/MZ files decrypt in place.
3. Enable **Reconstruct project** to generate the project files for the
   editor (MV/MZ requires an output directory for this).
4. Enable **Overwrite existing files** when the output directory should be
   reused.
5. **Start**, watch the progress and log, **Cancel** anytime.

The detailed decryption/compatibility notes are in the
[C++ README](RPGMakerDecrypter.Cpp/README.md#feature-parity-notes).

## Building the original C# CLI

The reference implementation lives under [legacy/](legacy/):

```bash
dotnet build -c Release legacy/RPGMakerDecrypter.sln
dotnet run --project legacy/RPGMakerDecrypter.Cli -- <archive-or-directory>
```

See the [original README](https://github.com/uuksu/RPGMakerDecrypter#usage)
for CLI options.

## Acknowledgments

- [Mikko Uuksulainen (uuksu)](https://github.com/uuksu) — original
  RPGMakerDecrypter and the archive format research it is based on.
- RPG Maker archive decryption algorithms are well documented by the
  community; this project reimplements them (see `RPGMakerDecrypter.Tests`
  for compatibility fixtures).

## License

MIT — same as the original project. See [LICENSE.txt](LICENSE.txt).
