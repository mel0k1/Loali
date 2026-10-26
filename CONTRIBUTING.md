# Contributing to Loali

Thanks for your interest in Loali! Contributions of any size are welcome — bug fixes, new shell features, design polish and packaging.

## Build from source

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/loali
```

Dependencies: Qt 6.4+ (Quick, QuickControls2, WaylandCompositor), CMake 3.21+, Ninja, Wayland dev libraries. CI runs on Ubuntu 24.04 and is the reference build environment.

## Ground rules

- **Commits are short.** Use brief lowercase titles: `shell`, `dock`, `fix focus`, `ci`. No long descriptions.
- **Code stays clean.** No comments, no hint texts, no debug noise in C++ and QML. The code explains itself.
- **Follow Material 3.** New UI must use the tokens from `Theme.qml` (colors, corner radii, emphasized easing). Avoid hardcoded values outside tokens.
- **One concern per commit.** Keep changes focused so review is easy.

## Areas that need help

- Virtual workspaces and window tiling
- StatusNotifier tray support in the taskbar
- Dynamic wallpaper color extraction (Material You)
- Distro packaging (Arch, Fedora, Nix)

## Reporting issues

Open an issue with your distro, Qt version (`qmake6 --version`), GPU/driver info and steps to reproduce. Logs from running `loali` in a terminal are highly appreciated.
