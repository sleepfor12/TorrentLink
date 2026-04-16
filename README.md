## 项目简介

这是一个基于 **Qt 6（Widgets）+ libtorrent-rasterbar（2.x）** 的桌面端 P2P 下载器项目，目标是实现一个稳定、可扩展、易维护的 BitTorrent 下载器。当前以 Linux 为主线开发，Windows 适配已启动并接入 CI 构建。

当前仓库已具备：
- 完整的 BitTorrent 下载管理（添加 magnet / .torrent、暂停/恢复/删除、限速）
- 多标签/分类管理与标签过滤
- 文件优先级与内容树
- Tracker 管理（添加/编辑/移除/强制汇报）
- Peer 列表与 HTTP 源列表
- RSS 订阅自动下载
- 系统托盘与下载完成桌面通知
- 搜索功能（本地历史 / RSS 条目检索）
- 速度图表与日志中心
- 持久化与 Resume Data 断点续传
- CMake 工程骨架（跨平台依赖发现：Linux `pkg-config` 回退，Windows 优先 CMake/vcpkg）
- GoogleTest 单元测试（`ctest` 可运行，196 测试用例）

相关文档：
- `docs/GETTING_STARTED.md`：**开发上手**（环境、构建、当前代码结构说明）
- `docs/PRD.md`：功能设计；**第 1.4 节**为与代码同步的实现状态摘要
- `docs/ARCHITECTURE.md`：架构与线程模型（含实现对照表）
- `docs/SYSTEM_DESIGN_OVERVIEW.md`：概要设计；**第 2.1.1 节**为设计名与源码模块映射
- `docs/NEXT_STEPS_UI_PLAN.md`：UI 与后续迭代建议（随版本更新）
- `docs/PREFERENCES_OPTIMIZATION_PLAN.md`：首选项扩展与高优化落地清单（配置键、优先级、验收标准）
- `docs/PREFERENCES_IMPLEMENTATION_STATUS.md`：首选项改造阶段总结（已完成 / 未打通 / 下一步）
- `docs/REQUIREMENTS_ANALYSIS.md`：需求分析（干系人/用例/功能与非功能/风险/验收；实现对照见 PRD 1.4）
- `docs/DEVELOPMENT_DEEP_DIVE.md`：深度剖析（线程/并发/时序图/风险；文首含命名映射说明）

开发辅助脚本：
- 一键构建与测试：`bash scripts/build.sh`（推荐开发前先跑通）
- 格式化：`bash scripts/format.sh`
- 格式检查（CI 用）：`bash scripts/format_check.sh`
- 代码风格由根目录 `.clang-format` 统一；CI 见 `.github/workflows/ci.yml`

---

## 架构概览

### 入口

应用入口为 `src/main.cc`，创建 `QApplication`、`SessionWorker`、`TaskPipelineService` 和 `AppController`。

### 分层

- **UI 层**（`src/ui/`）：`MainWindow`、`TransferPage`、`TaskDetailPanel`（含 `GeneralDetailPage`、`TrackerDetailPage`、`PeerListPage`、`HttpSourcePage`、`ContentTreePage`、`SpeedChartPage`）、`SearchTab`、`SystemTray`、RSS 模块页面、设置对话框等。
- **Core 层**（`src/core/`）：`TaskPipelineService`（事件管线）、`TaskStateStore`（状态合并）、`TaskSnapshot`（数据契约）、`TaskEvent`、RSS 服务等。
- **Libtorrent 适配层**（`src/lt/`）：`SessionWorker`（独立线程封装 `lt::session`）、`TaskAlertAdapter`（alert → `LtAlertView`）、`TaskEventMapper`（`LtAlertView` → `TaskEvent`）。
- **Base 层**（`src/base/`）：通用工具（格式化、时间戳、日志、类型定义等）。

### 线程模型

- **UI 线程**：Qt 事件循环，接收快照刷新 UI
- **SessionWorker 线程**：独立线程运行 `lt::session`，通过命令队列接收操作指令（add/pause/remove/query 等），通过回调将 alert 批次投递回 UI 线程

---

## 术语

- **任务/种子（Task/Torrent）**：一个 `.torrent` 或 magnet 对应的下载对象
- **Session**：libtorrent 的全局会话，管理网络、DHT、磁盘 IO 等
- **Alert**：libtorrent 异步事件（状态、错误、tracker、DHT 等）
- **Resume Data**：用于断点续传的任务恢复数据（核心）

---

## 构建与运行（Linux）

### 依赖

- Qt6：`qt6-base-dev qt6-declarative-dev`
- libtorrent：`libtorrent-rasterbar-dev`（2.x）
- pkg-config：`pkg-config`
- 编译器：支持 C++17 的 GCC 或 Clang

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

---

## Windows 适配状态

- 已纳入 CI：`windows-latest` 构建任务（Qt + vcpkg libtorrent）。
- 当前目标：先保证稳定编译，再逐步推进运行时行为与 Linux 对齐。
- 本地构建建议：使用 CMake + vcpkg（`CMAKE_TOOLCHAIN_FILE` 指向 vcpkg 工具链）。
- 运行时推进（进行中）：高级网络项已接通第一批（监听端口、端口转发开关、上传槽位、代理参数）。

---

## V2 Backlog（未实现）

以下功能为长期规划，尚未完整实现：
- IP 过滤/黑白名单（当前仅设置项，runtime 文件加载待补齐）
- 远程 Web UI / JSON-RPC
- 自动归档（完成后按分类移动）
- 在线种子搜索
- 诊断包导出
- 开机自启
