## 项目简介

English version: [README.md](README.md)

本项目为本人毕业设计，实现了一套轻量级P2P文件传输桌面应用，遵循MIT协议，许可证: [LICENSE](LICENSE)。

这是一个基于 **Qt 6（Widgets）+ libtorrent（2.0.5）** 的桌面端 P2P 下载器，目标是实现一个稳定、可扩展、易维护的 BitTorrent 客户端。
在部分 UI 设计上参考了 qBittorrent。

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
- **运行时：**会话侧网络相关选项已接通（监听端口、端口转发开关、上传槽位、代理、基于文本文件的 **IP 过滤**及规则文件变更后的热重载、队列上限等）。内置 HTTP Tracker 仍为实验性能力，见 [docs/HTTP_TRACKER_PLAN.md](docs/HTTP_TRACKER_PLAN.md)。

## V2 Backlog（未实现）

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