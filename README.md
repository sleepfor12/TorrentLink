## 项目简介

本项目为毕业设计，部分 UI 设计参考了 qBittorrent。

这是一个基于 **Qt 6（Widgets）+ libtorrent（2.6）** 的桌面端 P2P 下载器，目标是实现一个稳定、可扩展、易维护的 BitTorrent 客户端。当前以 Linux 为主线开发，Windows 已接入 CI 构建。

## 已实现功能

- BitTorrent 下载管理（添加 magnet / `.torrent`、暂停/恢复/删除、限速）
- 多标签/分类管理与标签过滤
- 文件优先级与内容树
- Tracker 管理（添加/编辑/移除/强制汇报）
- Peer 列表与 HTTP 源列表
- RSS 订阅自动下载
- 搜索（本地历史 / RSS 条目检索）
- 系统托盘、下载完成通知、速度图表、日志中心
- 持久化与 Resume Data 断点续传
- CMake 跨平台工程（Linux `pkg-config` 回退，Windows 优先 CMake/vcpkg）
- GoogleTest 单元测试（`ctest` 可运行，当前 196 用例）

## 项目依赖说明

### 核心依赖（必须）

- `libtorrent-rasterbar`
  - 用途：提供 BitTorrent 协议栈、会话管理、Peer/Tracker 通信、断点续传能力。

### 构建辅助依赖（推荐）

- `pkg-config`
  - 用途：在 Linux 上辅助发现第三方库路径（作为依赖发现回退机制）。

### 测试与开发依赖

- `GoogleTest`（通过工程配置启用）
  - 用途：单元测试与回归验证，使用 `ctest` 统一执行。
- `.clang-format`
  - 用途：统一代码风格；配合格式化脚本与 CI 检查。

## 快速构建与运行

### 构建（含测试）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

### 运行

```bash
./build/p2p_downloader "magnet:?xt=urn:btih:..."
```

## 开发辅助脚本

- 一键构建与测试：`bash scripts/build.sh`
- 格式化：`bash scripts/format.sh`
- 格式检查（CI 使用）：`bash scripts/format_check.sh`

## Windows 适配状态

- 已纳入 CI：`windows-latest`（Qt + vcpkg libtorrent）。
- 当前目标：先保证稳定编译，再逐步推进运行时行为与 Linux 对齐。
- 本地构建建议：使用 CMake + vcpkg（`CMAKE_TOOLCHAIN_FILE` 指向 vcpkg 工具链）。
- 运行时推进中：高级网络项已接通第一批（监听端口、端口转发开关、上传槽位、代理参数）。

## V2 Backlog（未实现）

- IP 过滤/黑白名单（当前仅设置项，runtime 文件加载待补齐）
- 远程 Web UI / JSON-RPC
- 自动归档（完成后按分类移动）
- 在线种子搜索
- 诊断包导出
- 开机自启

