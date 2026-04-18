## Overview

中文版: [READEME_zh.md](READEME_zh.md)

This project is my graduation design project, implementing a lightweight desktop P2P file transfer application, and it is open-sourced under the MIT License, with license details in [LICENSE](LICENSE).

It is a desktop P2P downloader based on **Qt 6 (Widgets) + libtorrent (2.0.5)**, aiming to provide a stable, extensible, and maintainable BitTorrent client.
Parts of the UI design reference qBittorrent.

## Implemented Features

- BitTorrent task management (add magnet / `.torrent`, pause/resume/delete, speed limits)
- Multi-tag/category management and tag filtering
- File priority and content tree
- Tracker management (add/edit/remove/force reannounce)
- Peer list and HTTP source list
- RSS subscription with auto-download
- Search (local history / RSS entry search)
- System tray, download completion notifications, speed chart, and log center
- Persistence and resume data support
- Cross-platform CMake project layout (Linux `pkg-config` fallback, Windows prefers CMake/vcpkg)
- GoogleTest unit tests

## Development Environment

- OS: `Ubuntu 22.04.5 LTS`
- Compiler (GCC): `13.1.0`
- Compiler (Clang): `18.1.8`
- CMake: `3.22.1`
- Qt (qmake6): `6.2.4`
- libtorrent (`pkg-config`): `2.0.5`
- GoogleTest

Notice: [NOTICE](NOTICE)

## Quick Build and Run

### Build (with tests)

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## Development Helper Scripts

- One-command build and test: `bash scripts/build.sh`
- Format code: `bash scripts/format.sh`
- Format check (used in CI): `bash scripts/format_check.sh`

## Bug and Vulnerability Reporting Guide

- For regular bugs: please submit a GitHub Issue and include reproduction steps, expected behavior, and actual behavior.
- For security vulnerabilities: please **do not** submit a public Issue.
- Please report vulnerabilities privately via email: `zbysleepallday@outloo.com`.
- It is recommended to include affected version/commit, impact scope, and a minimal reproduction or PoC.

## Windows Adaptation Status

- Included in CI: `windows-latest` (Qt + vcpkg libtorrent); **Release 构建后运行 `ctest`**（与 Linux 一样拉取 GoogleTest）。
- Current goal: keep compilation and tests green, then gradually align runtime behavior with Linux.
- Recommended local build setup: CMake + vcpkg (`CMAKE_TOOLCHAIN_FILE` points to the vcpkg toolchain); pass `-DCMAKE_PREFIX_PATH` to your Qt kit if `find_package(Qt6)` fails.
- Runtime progress (in progress): first batch of advanced network options is connected (listening port, port forwarding toggle, upload slots, proxy parameters).
- Optional embedded HTTP Tracker (experimental, P0): process-local `GET /announce` on `127.0.0.1` (see [docs/HTTP_TRACKER_PLAN.md](docs/HTTP_TRACKER_PLAN.md)); managed from app settings and `AppController`, not from the libtorrent session worker.

## V2 Backlog (Not Implemented Yet)

- IP filtering allow/block list (currently settings only, runtime file loading is pending)
- Remote Web UI / JSON-RPC
- Auto-archive (move completed tasks by category)
- Online torrent search
- Diagnostic bundle export
- Auto-start on boot

## Author Information

Author: sleepfor12

Email: [zbysleepallday@outloo.com](mailto:zbysleepallday@outloo.com) / [zbymeiqian414@163.com](mailto:zbymeiqian414@163.com)

## Acknowledgements

Thanks to my university and supervisor for their support during the graduation project.
