# 内置 HTTP Tracker 规划

本文描述在 TorrentLink 中实现「可选嵌入式 HTTP Tracker」的目标、边界与分阶段路线，供实现与评审使用。当前版本**未实现** Tracker 服务；设置项仅持久化，运行时见 `[prefs-runtime] builtin_tracker is not implemented` 日志。

## 1. 目标与边界

| 维度 | 说明 |
|------|------|
| **目标** | 在单机或局域网内提供符合 **BEP 3 / BEP 15** 的 HTTP(S) Tracker，使同一网络内的 torrent 能通过 `announce` / `scrape` 发现对端。 |
| **非目标** | 不做公网大规模 Tracker、不做账户体系与 Web 管理台（可与既有「远程 Web UI」 backlog 区分）。 |
| **与 libtorrent 关系** | 下载会话仍由 `SessionWorker` + libtorrent 承担；Tracker 为**独立 HTTP 服务**（或独立线程 + `QTcpServer`/`QHttpServer`），通过标准 HTTP 与客户端交互，**不**把 libtorrent 当作 Tracker 实现。 |

## 2. 架构选项（择一或组合）

### 方案 A：进程内嵌入式 HTTP 服务（推荐首选评估）

- **实现**：在应用进程内用 **Qt Network**（`QTcpServer` + 手写极简 HTTP）或 **QHttpServer**（若 Qt 版本与打包允许）监听 `builtin_tracker_port`（0 表示由 OS 选口）。
- **线程**：独立 `QThread` 或 `asio` 线程，**禁止**在 `SessionWorker` 的 libtorrent 线程里处理 HTTP。
- **数据**：内存维护 `info_hash -> peers`（IP、port、peer id、seed/leech）；定期过期；与 libtorrent 会话**可选**通过本机回调同步「真实连接数」（进阶，非 P0）。

### 方案 B：独立辅助进程

- 小可执行文件 + 与主程序 IPC（本地 socket / 共享配置）。  
- **优点**：崩溃隔离、端口与防火墙策略清晰。  
- **缺点**：打包、安装与调试成本更高。

### 方案 C：仅文档与外链

- 不内置服务，设置里改为「使用外部 Tracker URL」或移除开关。  
- 适合若产品决定不做局域网 Tracker。

## 3. 分阶段交付

| 阶段 | 内容 | 验收 |
|------|------|------|
| **P0** | 需求冻结：仅 IPv4 监听、HTTP（可先不做 HTTPS）、`announce` GET 最小字段、`compact=1`、简单 `failure reason`。 | curl / 浏览器能拿到合法 bencode 响应；主程序日志显示监听端口。 |
| **P1** | `scrape`、peer 过期、与设置中 `builtin_tracker_port` / `builtin_tracker_port_forwarding` 联动（UPnP 映射可与现有会话逻辑协调或单独映射）。 | 两个本机客户端同一 info_hash 能互相发现（或单客户端自测 announce 入库）。 |
| **P2** | 绑定地址、访问控制（仅 127.0.0.1 / 局域网段）、基础限流与日志。 | 误暴露公网风险可控；README 说明安全建议。 |
| **P3** | UI：展示「Tracker 运行中 / 已停止 / 端口」、一键复制 announce URL。 | 用户无需手拼 URL。 |

## 4. 配置项映射（与现有 `AppSettings` 对齐）

- `protocol/builtin_tracker_enabled`：总开关。  
- `protocol/builtin_tracker_port`：监听端口；0 表示自动。  
- `protocol/builtin_tracker_port_forwarding`：是否请求路由器映射（与实现方案绑定，可能复用 libtorrent 的 `session` 端口映射器或独立 miniupnp 调用）。  
- `protocol/monitor_port`：与 Tracker **解耦**；当前语义为额外 `listen_interfaces`，**不要**与 Tracker 端口混用，除非产品重新定义。

## 5. 风险与缓解

| 风险 | 缓解 |
|------|------|
| 公网误监听 | 默认仅绑定 `127.0.0.1` 或明确「仅局域网」模式；文档警示。 |
| 与 libtorrent 端口/防火墙冲突 | Tracker 使用独立端口；CI 增加 Windows/Linux 冒烟测试（启动/停止）。 |
| 维护成本 | 优先最小 BEP 子集；依赖清晰单元测试（HTTP 解析与 bencode 响应）。 |

## 6. 建议的后续任务（实现前）

1. 在 `docs/` 或 Issue 中固定 **P0 请求/响应示例**（含 sample torrent 的 announce URL）。  
2. 决定采用方案 A 或 B，并更新 `README`「内置 Tracker」条目状态。  
3. Windows CI 已跑单元测试后，为 Tracker 增加 **无头集成测试**（仅当 `BUILD_TESTING=ON` 且可选 `TORRENTLINK_ENABLE_TRACKER_TESTS`）。

---

修订记录：初版对应「设置项已存在、服务未实现」之规划；随实现进度更新阶段状态。
