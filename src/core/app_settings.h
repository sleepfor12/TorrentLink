#ifndef PFD_CORE_APP_SETTINGS_H
#define PFD_CORE_APP_SETTINGS_H

#include <QtCore/QString>

namespace pfd::core {

struct AppSettings {
  QString default_download_dir;

  bool seed_unlimited{false};
  double seed_target_ratio{1.00};
  int seed_max_minutes{0};  // 0=不限制

  bool file_log_enabled{true};
  QString log_dir;
  QString log_level{QStringLiteral("info")};
  qint64 log_rotate_max_file_size_bytes{0};  // <=0 关闭轮转
  int log_rotate_max_backup_files{3};

  int download_rate_limit_kib{0};         // 0=不限速
  int upload_rate_limit_kib{0};           // 0=不限速
  int connections_limit{200};             // 0=默认/不限
  int per_torrent_connections_limit{0};   // 0=不限（每个 torrent）
  int upload_slots_limit{0};              // 0=默认/不限（全局上传窗口数）
  int per_torrent_upload_slots_limit{0};  // 0=默认/不限（每个 torrent 上传窗口数）
  int listen_port{0};                     // 0=自动/禁用指定端口
  bool listen_port_forwarding{true};      // 使用路由器 UPnP/NAT-PMP 端口转发

  // --- 队列/并发（0 表示不限制/使用默认） ---
  int active_downloads{0};
  int active_seeds{0};
  int active_limit{0};
  int max_active_checking{1};
  bool auto_manage_prefer_seeds{false};
  bool dont_count_slow_torrents{true};
  int slow_torrent_dl_rate_threshold{2};  // KiB/s
  int slow_torrent_ul_rate_threshold{2};  // KiB/s
  int slow_torrent_inactive_timer{60};    // seconds

  // --- 协议/发现 ---
  bool enable_dht{true};
  bool enable_upnp{true};
  bool enable_natpmp{true};
  bool enable_lsd{true};
  int monitor_port{0};  // 0=禁用
  bool builtin_tracker_enabled{false};
  int builtin_tracker_port{0};  // 0=自动/禁用指定端口
  bool builtin_tracker_port_forwarding{false};

  // --- 加密策略 ---
  // "enabled" | "forced" | "disabled"
  QString encryption_mode{QStringLiteral("enabled")};

  // --- 网络高级 ---
  bool ip_filter_enabled{false};
  QString ip_filter_path;
  bool proxy_enabled{false};
  // "socks5" | "http"
  QString proxy_type{QStringLiteral("socks5")};
  QString proxy_host;
  int proxy_port{1080};
  QString proxy_username;
  QString proxy_password;

  // --- 界面行为 ---
  // "minimize" | "quit"
  QString close_behavior{QStringLiteral("minimize")};
  bool start_minimized{false};

  // --- 定时动作 ---
  // "none" | "quit_app" | "shutdown"
  QString timed_action{QStringLiteral("none")};
  int timed_action_delay_minutes{0};  // 0=禁用

  // --- 全部任务下载完成后的动作 ---
  // "none" | "quit_app" | "suspend" | "hibernate" | "poweroff"
  // 当 timed_action != "none" 且 delay>0 时，此字段会被强制视为 "none"。
  QString download_complete_action{QStringLiteral("none")};

  // --- RSS/搜索请求头 ---
  QString http_user_agent{QStringLiteral("TorrentLink/1.0")};
  QString http_accept_language{QStringLiteral("zh-CN,zh;q=0.9,en;q=0.8")};
  QString http_cookie_header;
  // 每行一条规则：domain<TAB>name=value
  QString http_cookie_rules;

  static QString settingsFilePath();

  static AppSettings load();
  static void save(const AppSettings& s);
};

}  // namespace pfd::core

#endif  // PFD_CORE_APP_SETTINGS_H
