## 项目简介

English version: [README.md](README.md)

本项目为本人毕业设计，实现了一套轻量级P2P文件传输桌面应用，遵循MIT协议，许可证: [LICENSE](LICENSE)。

这是一个基于 **Qt 6（Widgets）+ libtorrent（2.0.5）** 的桌面端 P2P 下载器，目标是实现一个稳定、可扩展、易维护的 BitTorrent 客户端。
在部分 UI 设计上参考了 qBittorrent。

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
- GoogleTest 单元测试

## 开发环境

- 操作系统：`Ubuntu 22.04.5 LTS`
- 编译器（GCC）：`13.1.0`
- 编译器（Clang）：`18.1.8`
- CMake：`3.22.1`
- Qt（qmake6）：`6.2.4`
- libtorrent（`pkg-config`）：`2.0.5`
- GoogleTest

声明: [NOTICE](NOTICE)

## 快速构建与运行

### 构建（含测试）

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DBUILD_TESTING=ON
cmake --build build -j
ctest --test-dir build --output-on-failure
```

## 开发辅助脚本

- 一键构建与测试：`bash scripts/build.sh`
- 格式化：`bash scripts/format.sh`
- 格式检查（CI 使用）：`bash scripts/format_check.sh`

## Bug 与漏洞报告指南

- 普通 Bug：请通过 GitHub Issue 提交，并附上复现步骤、预期行为、实际行为。
- 安全漏洞：请 **不要** 公开提交 Issue。
- 请通过邮箱私下报告：`zbysleepallday@outloo.com`。
- 建议附上受影响版本/commit、影响范围、最小复现或 PoC。

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

## 作者信息

作者：sleepfor12

邮箱：[zbysleepallday@outloo.com](mailto:zbysleepallday@outloo.com) / [zbymeiqian414@163.com](mailto:zbymeiqian414@163.com)

## 致谢

感谢母校及指导老师在毕业设计期间帮助。