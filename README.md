<div align="center">
  <img src="assets/banner.svg" alt="Hi! Loali" width="820"/>
</div>

<div align="center">

# Hi! Loali

**Material 3 Expressive desktop environment for Linux — built with Qt 6, QML and Wayland**

[![ci](https://github.com/mel0k1/Loali/actions/workflows/ci.yml/badge.svg)](https://github.com/mel0k1/Loali/actions/workflows/ci.yml)
[![License: GPL-3.0](https://img.shields.io/badge/license-GPL--3.0-blue.svg)](LICENSE)
[![Qt](https://img.shields.io/badge/Qt-6.4%2B-41CD52.svg)](https://www.qt.io/)
[![Wayland](https://img.shields.io/badge/Wayland-native-6E56A6.svg)](https://wayland.freedesktop.org/)
[![PRs Welcome](https://img.shields.io/badge/PRs-welcome-7D5260.svg)](CONTRIBUTING.md)

</div>

---

**Loali** is a lightweight, expressive desktop shell for Linux. It is a real Wayland compositor written in C++ and QML on top of Qt Wayland Compositor — the same technology used by production display servers. Instead of floating windows over an empty canvas, Loali brings a familiar and friendly layout: a **dock pinned to the top**, a **taskbar along the bottom**, and **icons living right on the desktop**, all wrapped in the bold shapes, vivid tonal colors and springy motion of Material 3 Expressive.

<!-- TODO: screenshots -->

## Features

- **Native Wayland compositor** — Loali *is* the display server, built with the official Qt Wayland Compositor API (XdgShell, server-side decorations, popups).
- **Top dock** — a floating pill with pinned apps, springy hover magnification, running indicators and an app grid launcher button.
- **Bottom taskbar** — window buttons for every open toplevel, a lock shortcut and a live clock.
- **Desktop icons** — a grid of installed applications directly on the wallpaper, with selection and double-click launch.
- **App launcher** — a searchable full-screen grid of installed applications.
- **Window management** — server-side title bars, drag, 8-way resize, maximize, fullscreen, minimize, click-to-focus.
- **Lock screen** — large M3 display-style clock over a dimmed wallpaper.
- **Material 3 Expressive design** — dark tonal palette, 18–32 px corner radii, emphasized easing curves and spring animations.
- **Automatic app discovery** — parses `.desktop` files from standard XDG locations, with a themed-icon provider and graceful fallback icons.

## Getting started

### Dependencies

| Distro | Packages |
| --- | --- |
| Debian / Ubuntu | `build-essential cmake ninja-build qt6-base-dev qt6-declarative-dev qt6-wayland-dev libwayland-dev` |
| Arch | `base-devel cmake ninja qt6-base qt6-declarative qt6-wayland wayland` |
| Fedora | `gcc-c++ cmake ninja-build qt6-qtbase-devel qt6-qtdeclarative-devel qt6-qtwayland-devel wayland-devel` |

Runtime QML modules (Debian/Ubuntu naming): `qml6-module-qtquick-controls`, `qml6-module-qtquick-layouts`, `qml6-module-qtquick-window`, `qml6-module-qtquick-shapes`, `qml6-module-qtwayland-compositor`.

### Build

```bash
git clone https://github.com/mel0k1/Loali.git
cd Loali
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

### Run

The easiest way to try Loali is a **nested window** inside your current session:

```bash
./build/loali
```

Launch Wayland clients against it with `WAYLAND_DISPLAY` printed by Loali at startup, for example:

```bash
WAYLAND_DISPLAY=wayland-1 QT_QPA_PLATFORM=wayland konsole
```

For a fullscreen nested window use `./build/loali --fullscreen`.

### Install as a session

```bash
sudo cmake --install build
```

This installs the `loali` binary, a `loali.desktop` application entry and a Wayland session file, so **Loali** appears in your display manager session list (SDDM, GDM, Ly, …).

## Architecture

```
Loali/
├── src/
│   ├── main.cpp              QGuiApplication + QML engine bootstrap
│   ├── apps.{h,cpp}          XDG .desktop scanner, model, pins, launcher
│   └── iconprovider.{h,cpp}  themed QIcon image provider with M3 fallback
├── qml/
│   ├── Main.qml              WaylandCompositor, XdgShell, window routing
│   ├── Chrome.qml            server-side window decorations, drag/resize
│   ├── Dock.qml              top dock (pinned apps, indicators)
│   ├── Taskbar.qml           bottom taskbar (windows, lock, clock)
│   ├── Desktop.qml           wallpaper + desktop icon grid
│   ├── Launcher.qml          app grid overlay with search
│   ├── LockScreen.qml        lock overlay with big clock
│   ├── Wallpaper.qml         procedural M3E gradient blobs
│   ├── Theme.qml             Material 3 design tokens (singleton)
│   ├── MIcon.qml             Material Symbols path renderer
│   └── Shadow.qml            layered soft shadow
└── assets/
```

The shell follows the reference Qt compositor pattern: `XdgShell.onToplevelCreated` creates a `Chrome` (a `ShellSurfaceItem` wrapped in a rounded container with a title bar). Windows are tracked in a `ListModel` that drives both the taskbar and the dock, and `XdgDecorationManagerV1` requests server-side decorations so client windows integrate seamlessly with the M3 look.

## Roadmap

- [x] Wayland compositor core with XdgShell
- [x] Top dock, bottom taskbar, desktop icons
- [x] App launcher with search
- [x] Lock screen
- [ ] Virtual workspaces
- [ ] Window tiling and snapping
- [ ] StatusNotifier tray support
- [ ] Dynamic Material You color extraction from wallpaper
- [ ] Notifications daemon
- [ ] Settings application

## Contributing

Contributions are welcome! Please read [CONTRIBUTING.md](CONTRIBUTING.md) — keep commits short, keep code clean and comment-free, and follow the Material 3 Expressive language.

## License

Loali is licensed under the [GNU GPL-3.0](LICENSE). Material Symbols path data used by `MIcon.qml` is derived from [Material Design Icons](https://github.com/google/material-design-icons) (Apache-2.0).

## Русский

**Loali** — лёгкая и выразительная графическая среда для Linux. Это настоящий Wayland-композитор на C++ и QML поверх Qt Wayland Compositor. Знакомая раскладка в смелом стиле Material 3 Expressive: **док сверху**, **панель задач снизу**, **иконки прямо на рабочем столе**.

Возможности: нативный Wayland-композитор (XdgShell, серверные декорации), верхний док с закреплёнными приложениями и пружинными анимациями, нижний таскбар с кнопками окон и часами, сетка иконок на рабочем столе, лаунчер с поиском, перетаскивание, изменение размера с 8 сторон, разворачивание, сворачивание, экран блокировки с большими часами, автоматический поиск приложений по `.desktop`-файлам.

Сборка и запуск:

```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
./build/loali
```

Клиенты подключаются через `WAYLAND_DISPLAY`, который Loali печатает при старте. Установка сессии: `sudo cmake --install build` — после этого Loali появится в списке сессий менеджера входа.

Лицензия — [GPL-3.0](LICENSE).
