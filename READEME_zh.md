## 项目简介

English version: [README.md](README.md)

本项目为本人毕业设计，实现了一套轻量级P2P文件传输桌面应用，遵循MIT协议，许可证: [LICENSE](LICENSE)。

这是一个基于 **Qt 6（Widgets）+ libtorrent（2.0.5）** 的桌面端 P2P 下载器，目标是实现一个稳定、可扩展、易维护的 BitTorrent 客户端。
在部分 UI 设计上参考了 qBittorrent。

## 功能概览

- **传输：**打开 `.torrent` 与磁力链接；暂停/继续；分类、标签与队列控制。
- **任务详情**（视图 → 显示传输页任务详情）：普通、Tracker、用户、**HTTP源**（Web Seed，BEP17/BEP19）、内容、速度。
- **RSS：**订阅源、规则、自动下载、OPML 导入/导出。
- **网络（工具 → 首选项 → 连接）：**限速、监听端口、UPnP/NAT-PMP、连接加密；**SOCKS5/HTTP 代理**（BitTorrent 会话 + RSS/HTTP）；**IP 过滤**（文本规则文件，支持热重载）。
- **内置 HTTP Tracker（实验性）：**嵌入式 BEP3 `GET /announce`；绑定 **仅本机** 或 **局域网**（首选项 → 连接 → 发现与端口映射）。UPnP/NAT-PMP 端口映射尚未实现。
- **工具：**生成 Torrent（可填 Web Seed）、管理 Cookies、日志中心、浅色/深色/跟随系统主题。

## 开发环境

- 操作系统：`Ubuntu 22.04.5 LTS`
- 编译器（GCC）：`13.1.0`
- 编译器（Clang）：`18.1.8`
- CMake：`3.22.1`
- Qt（qmake6）：`6.2.4`
- libtorrent（`pkg-config`）：`2.0.5`
- GoogleTest

声明: [NOTICE](NOTICE)

## 依赖

### Linux（Ubuntu/Debian）

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

### Windows（MSVC + vcpkg）

1. 安装 Qt 6 Desktop 套件（与本地 CMake 生成器/编译器匹配）。
2. 初始化 vcpkg 并按仓库 manifest 安装依赖：

```powershell
git clone https://github.com/microsoft/vcpkg.git C:/vcpkg
C:/vcpkg/bootstrap-vcpkg.bat -disableMetrics
cd <repo-root>
C:/vcpkg/vcpkg.exe install --triplet x64-windows
```

3. 通过 vcpkg 工具链与 Qt 路径配置 CMake：

```powershell
cmake -S . -B build-win `
  -DCMAKE_BUILD_TYPE=Release `
  -DBUILD_TESTING=ON `
  -DCMAKE_TOOLCHAIN_FILE=C:/vcpkg/scripts/buildsystems/vcpkg.cmake `
  -DVCPKG_TARGET_TRIPLET=x64-windows `
  -DCMAKE_PREFIX_PATH="<your-qt-root>"
```

## 快速构建与运行

### 构建（含测试）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### 测试目标拆分

- 核心/非 UI 测试：`torrentlink_tests`
- UI 测试：`torrentlink_ui_tests`

## 开发辅助脚本

- 一键构建与测试：`bash scripts/build.sh`
- 格式化：`bash scripts/format.sh`
- 格式检查（CI 使用）：`bash scripts/format_check.sh`

## Bug 与漏洞报告指南

- 普通 Bug：请通过 GitHub Issue 提交，并附上复现步骤、预期行为、实际行为。
- 安全漏洞：请 **不要** 公开提交 Issue。
- 请通过邮箱私下报告：`zbysleepallday@outlook.com`。
- 建议附上受影响版本/commit、影响范围、最小复现或 PoC。

## Windows 适配状态

- **CI：**`windows-latest`（Qt + vcpkg libtorrent）；Release / Debug 构建后与 Linux 一样执行 **`ctest`**（GoogleTest）。
- **现状：**Windows 下编译与自动化测试已稳定；当前版本的**本地 Windows 实测已基本完成**，剩余多为与 Linux 的细节对齐与体验优化，而非从零适配。
- **本地构建：**CMake + vcpkg（`CMAKE_TOOLCHAIN_FILE`）；若找不到 Qt，请设置 `-DCMAKE_PREFIX_PATH` 指向本机 Qt 安装根目录。
- **运行时：**会话侧网络相关选项已接通（监听端口、端口转发开关、上传槽位、**SOCKS5/HTTP 代理**、基于文本文件的 **IP 过滤**及规则文件变更后的热重载、队列上限等）。内置 HTTP Tracker 为实验性能力：在 **工具 → 首选项 → 连接 → 发现与端口映射** 中启用并选择绑定范围（仅本机 / 局域网）；保存后在日志中查找 `[builtin-tracker]` 或 `builtin tracker ready` 获取 announce URL。Tracker 由 `AppController` 管理，不在 libtorrent 会话线程内运行。

## 测试 HTTP 源面板

1. 编译运行后，打开带有 Web Seed 的 torrent（元信息中含 `url-list` 或 `httpseeds`）。
2. 在 **传输** 页选中任务；勾选 **视图 → 显示传输页任务详情**。
3. 在底部详情栏打开 **HTTP源** Tab，应看到 URL 与类型（BEP17 / BEP19）两列。

也可通过 **工具 → 生成 Torrent**，在 Web Seed 栏每行填一个 URL，生成 `.torrent` 后自行测试。

## V2 Backlog（未实现）

- 内置 HTTP Tracker：UPnP/NAT-PMP 端口映射、`scrape`、状态 UI
- IP 过滤扩展（如 eMule `ipfilter.dat` 二进制列表、传输列表右键「禁止该 IP」写入规则等）
- 远程 Web UI / JSON-RPC
- 自动归档（完成后按分类移动）
- 在线种子搜索
- 诊断包导出
- 开机自启

## 作者信息

作者：sleepfor12

邮箱：[zbysleepallday@outlook.com](mailto:zbysleepallday@outlook.com) / [zbymeiqian414@163.com](mailto:zbymeiqian414@163.com)

## 致谢

感谢母校及指导老师在毕业设计期间帮助。