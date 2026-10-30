# RPG Maker Decrypter (C++ / Qt 6)

A cross-platform GUI rewrite of RPG MakerDecrypter in C++ using Qt 6.
It can extract encrypted archives and files created with **RPG Maker XP, VX, VX Ace, MV or MZ**,
and it can reconstruct a best guess of the original project so it can be opened in the editor again.

## Features

- Extract **RGSSAD archives** from RPG Maker XP (`.rgssad`), VX (`.rgss2a`) and
  VX Ace (`.rgss3a`), including archives that have been renamed. The archive format
  version is detected from the file header.
- Decrypt **RPG Maker MV and MZ deployments** (Windows, Linux, web and macOS `.app`
  bundles), using the encryption key stored in `System.json`.
- **Reconstruct project files** for the RPG Maker editors:
  - XP/VX/VX Ace: `Game.rxproj` / `Game.rvproj` / `Game.rvproj2` + `Game.ini`
  - MV: `Game.rpgproject`
  - MZ: `game.rmmzproject`
- Interactive GUI with automatic version detection, progress reporting and cancellation.

## Building

Requirements: a C++17 compiler, CMake 3.21+ and Qt 6.4+ (Widgets and Core modules).

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
```

On Debian/Ubuntu the development packages can be installed with:

```bash
sudo apt install build-essential cmake qt6-base-dev
```

The build produces:

- `RPGMakerDecrypter` – the GUI application
- `RPGMakerDecrypterTests` – self-tests for the decryption core

Run the self-tests with:

```bash
cd build && ctest
```

## Using the application

1. Select the input with **Browse File...** (an RGSSAD archive) or
   **Browse Folder...** (an MV/MZ deployment directory). The detected RPG Maker
   version is shown automatically.
2. Optionally select an output directory. When left empty:
   - archive contents are extracted next to the archive
   - MV/MZ files are decrypted in place next to the encrypted files
3. Enable **Reconstruct project** to generate the project files for the editor.
   For MV and MZ, an output directory is required.
4. Enable **Overwrite existing files** to replace existing output files
   (required when the output directory already exists).
5. Press **Start**. Progress and messages are shown in the log; the operation
   can be cancelled at any time.

## Project layout

```
src/core     decryption core library (Qt Core only, no GUI dependencies)
             - RGSSADV1Archive / RGSSADV3Archive   RGSSAD archive formats
             - VersionFinder                       automatic version detection
             - EncryptionKeyFinder / FileDecrypter MV/MZ file decryption
             - DirectoryFilesDecrypter             MV/MZ directory decryption
             - ProjectReconstructor                MV/MZ project reconstruction
             - ProjectGenerator                    XP/VX/VX Ace project files
src/app      Qt Widgets GUI application
tests        self-tests that round-trip synthetic fixtures
```

## Feature parity notes

This rewrite is functionally equivalent to the original C# CLI, with a few
deliberate, safe deviations:

- Archives renamed to non-standard extensions are still opened by reading the
  format version byte from the header instead of relying on the file extension.
- The project reconstruction refuses to overwrite the original deployment
  directory with itself instead of deleting it first.
- The GUI reports files that fail to decrypt instead of stopping silently, and
  every long operation can be cancelled.

## License

Distributed under the MIT license, same as the original project. See `../LICENSE.txt`.
