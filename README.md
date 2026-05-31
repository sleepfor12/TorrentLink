## Overview

中文版: [READEME_zh.md](READEME_zh.md)

This project is my graduation design project, implementing a lightweight desktop P2P file transfer application, and it is open-sourced under the MIT License, with license details in [LICENSE](LICENSE).

It is a desktop P2P downloader based on **Qt 6 (Widgets) + libtorrent (2.0.5)**, aiming to provide a stable, extensible, and maintainable BitTorrent client.
Parts of the UI design reference qBittorrent.

## Features

- **Transfers:** open `.torrent` files and magnet links; pause/resume; categories, tags, and queue controls.
- **Task detail panel** (View → show transfer task details): General, Tracker, Peers, **HTTP sources** (Web Seed URLs, BEP17/BEP19), Content, Speed.
- **RSS:** feeds, rules, auto-download, OPML import/export.
- **Network (Preferences → Connection):** rate limits, listen port, UPnP/NAT-PMP, connection encryption; **SOCKS5/HTTP proxy** (BitTorrent session + RSS/HTTP); **IP filter** (text rule file with hot-reload).
- **Built-in HTTP Tracker (experimental):** embedded BEP3 `GET /announce` service; bind to **localhost** or **LAN** (`Tools → Preferences → Connection → Discovery`). Port forwarding (UPnP/NAT-PMP) is not implemented yet.
- **Tools:** create torrent (including Web Seed URLs), cookie manager, log center, light/dark/system theme.

## Development Environment

- OS: `Ubuntu 22.04.5 LTS`
- Compiler (GCC): `13.1.0`
- Compiler (Clang): `18.1.8`
- CMake: `3.22.1`
- Qt (qmake6): `6.2.4`
- libtorrent (`pkg-config`): `2.0.5`
- GoogleTest

Notice: [NOTICE](NOTICE)

## Dependencies Installation (Linux/Windows)

### Linux (Ubuntu/Debian)

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential \
  cmake \
  pkg-config \
  qt6-base-dev \
  qt6-declarative-dev \
  libtorrent-rasterbar-dev
```

### Windows (MSVC + vcpkg)

1. Install Qt 6 desktop kit (same toolchain as your CMake generator).
2. Bootstrap vcpkg and install dependencies from manifest:

```powershell
git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
C:/vcpkg/bootstrap-vcpkg.bat -disableMetrics
cd <repo-root>
C:/vcpkg/vcpkg.exe install --triplet x64-windows
```

3. Configure CMake with vcpkg toolchain and Qt path:

```powershell
cmake -S . -B build-win `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTING=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_PREFIX_PATH="<your-qt-root>"
```

## Quick Build and Run

### Build (with tests)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### Build tests separately

- Core/non-UI tests: `torrentlink_tests`
- UI tests: `torrentlink_ui_tests`

## Development Helper Scripts

- One-command build and test: `bash scripts/build.sh`
- Format code: `bash scripts/format.sh`
- Format check (used in CI): `bash scripts/format_check.sh`

## Bug and Vulnerability Reporting Guide

- For regular bugs: please submit a GitHub Issue and include reproduction steps, expected behavior, and actual behavior.
- For security vulnerabilities: please **do not** submit a public Issue.
- Please report vulnerabilities privately via email: `zbysleepallday@outlook.com`.
- It is recommended to include affected version/commit, impact scope, and a minimal reproduction or PoC.

## Windows Adaptation Status

- **CI:** `windows-latest` with Qt + vcpkg libtorrent; Release (and Debug) builds run **`ctest`** the same way as on Linux (GoogleTest).
- **Status:** Windows build + automated tests are in good shape; hands-on Windows testing is **largely complete** for the current feature set. Remaining work is mostly polish and parity edge cases vs Linux, not greenfield porting.
- **Local build:** CMake + vcpkg (`CMAKE_TOOLCHAIN_FILE`); set `-DCMAKE_PREFIX_PATH` to your Qt installation if `find_package(Qt6)` fails.
- **Runtime:** Core session options are wired (listen port, port forwarding toggles, upload slots, **SOCKS5/HTTP proxy**, **IP filter** text file with hot-reload, queue limits, etc.). The optional embedded HTTP Tracker is experimental: enable it under **Tools → Preferences → Connection → Discovery** (`localhost` or `lan` bind); check logs for `[builtin-tracker]` / `builtin tracker ready` announce URLs. It runs in `AppController`, not on the libtorrent session worker thread.

## Testing HTTP Sources (Web Seed panel)

1. Build and run the app; open a torrent that includes Web Seeds (`url-list` / `httpseeds` in the metainfo).
2. Select the task on the **Transfer** tab; enable **View → Show transfer task details**.
3. Open the **HTTP sources** tab in the bottom detail bar — you should see URL and type columns (BEP17 / BEP19).

You can also use **Tools → Create torrent** and add one Web Seed URL per line when generating a `.torrent` for manual testing.

## Author Information

Author: sleepfor12

Email: [zbysleepallday@outlook.com](mailto:zbysleepallday@outlook.com) / [zbymeiqian414@163.com](mailto:zbymeiqian414@163.com)

## Acknowledgements

Thanks to my university and supervisor for their support during the graduation project.
