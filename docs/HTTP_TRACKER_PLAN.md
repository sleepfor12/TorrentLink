# 内置 HTTP Tracker 规划

本文描述在 TorrentLink 中实现「可选嵌入式 HTTP Tracker」的目标、边界与分阶段路线，供实现与评审使用。

**当前状态（方案 A / P0）**：已在应用进程内实现实验性嵌入式 HTTP Tracker（`BuiltinHttpTracker`），由 **`AppController` 根据 `AppSettings` 启停**；监听 **`127.0.0.1`**，端口来自设置（`0` 表示由 OS 选口，实际端口见日志前缀 **`[builtin-tracker]`**）。**不**再经 `SessionWorker` / libtorrent 应用 Tracker 相关设置。

## 1. 目标与边界

| 维度 | 说明 |
|------|------|
| **目标** | 在单机或局域网内提供符合 **BEP 3 / BEP 15** 的 HTTP(S) Tracker，使同一网络内的 torrent 能通过 `announce` / `scrape` 发现对端。 |
| **非目标** | 不做公网大规模 Tracker、不做账户体系与 Web 管理台（可与既有「远程 Web UI」 backlog 区分）。 |
| **与 libtorrent 关系** | 下载会话仍由 `SessionWorker` + libtorrent 承担；Tracker 为**独立 HTTP 服务**（独立线程 + `QTcpServer`），通过标准 HTTP 与客户端交互，**不**把 libtorrent 当作 Tracker 实现。 |

## 2. 架构选项（择一或组合）

### 方案 A：进程内嵌入式 HTTP 服务（**已采纳，P0 已实现最小子集**）

- **实现**：在 `src/core/` 内用 **Qt Network**（`QTcpServer` + 手写极简 HTTP），**未**使用 `QHttpServer` 模块。
- **线程**：独立 `QThread` + 工作 `QObject`，**禁止**在 `SessionWorker` 的 libtorrent 线程里处理 HTTP。
- **数据**：内存维护 `info_hash -> peers`（compact IPv4）；定期过期；与 libtorrent 会话无耦合。

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
| **P0** | 仅 **IPv4**、**HTTP**、`GET /announce`、**`compact=1`**（非 1 则 `failure reason`）、`failure reason` bencode、监听 **127.0.0.1**。 | 单元测试（bencode、查询解析）；本机 GET 冒烟；日志含 `http://127.0.0.1:<port>/announce`。 |
| **P1** | `scrape`、peer 策略与 `numwant`、**`builtin_tracker_port_forwarding`** 与 UPnP/NAT-PMP 联动（文档化二选一）。 | 两个本机客户端同一 info_hash 能互相发现（或单客户端自测 announce 入库）。 |
| **P2** | 绑定地址、访问控制（仅 127.0.0.1 / 局域网段）、基础限流与日志。 | 误暴露公网风险可控；README 说明安全建议。 |
| **P3** | UI：展示「Tracker 运行中 / 已停止 / 端口」、一键复制 announce URL。 | 用户无需手拼 URL。 |

## 4. P0 请求/响应示例（固定 info_hash 十六进制样例）

以下 **info_hash** 为 20 字节，全为 `0x01`（仅作文档示例；真实客户端为 torrent 的 20 字节原始 info_hash）。

**请求（节选）**

```http
GET /announce?info_hash=%01%01...（共20个%XX三元组）&peer_id=%2D%51%32...&port=6881&compact=1&left=0 HTTP/1.0
```

**成功响应体**（bencode 字典，键含 `interval`、`min interval`、`complete`、`incomplete`、`peers`；`peers` 为 **compact** 二进制串，每 6 字节为 IPv4 大端 + 端口大端）：

- 空 peers 时字典的字节序列形态为（可读性起见用注释标键名；实际为连续 bencode）：
  - `d8:intervali1800e12:min intervali900e8:completei0e10:incompletei0e5:peers0:e`
  - 注意键 **`peers`** 的 bencode 长度为 **`5:peers`**（五个字母），不是 `6:peers`。

## 5. 配置项映射（与现有 `AppSettings` 对齐）

- `protocol/builtin_tracker_enabled`：总开关。  
- `protocol/builtin_tracker_port`：监听端口；0 表示自动。  
- `protocol/builtin_tracker_port_forwarding`：是否请求路由器映射（**P0 未实现**；若开启，应用会记录说明性日志）。  
- `protocol/monitor_port`：与 Tracker **解耦**；当前语义为额外 `listen_interfaces`，**不要**与 Tracker 端口混用，除非产品重新定义。

## 6. 风险与缓解

| 风险 | 缓解 |
|------|------|
| 公网误监听 | P0 仅绑定 `127.0.0.1`；文档警示。 |
| 与 libtorrent 端口/防火墙冲突 | Tracker 使用独立端口；CI 增加冒烟测试（启动/停止）。 |
| 维护成本 | 最小 BEP 子集；依赖清晰单元测试（HTTP 解析与 bencode 响应）。 |

## 7. 建议的后续任务

1. P1：`scrape`、端口转发与 peer 策略。  
2. 若需局域网访问：增加绑定地址或「仅本机 / 所有接口」开关（默认安全值）。  
3. Windows CI 在 `BUILD_TESTING=ON` 下保持 Tracker 相关测试稳定。

---

修订记录：已采纳方案 A；P0 已实现并与 `AppController` 集成；历史「仅设置持久化、运行时未实现」描述已过期。
