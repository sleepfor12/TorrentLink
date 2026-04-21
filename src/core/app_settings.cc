#include "core/app_settings.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QStandardPaths>

#include <algorithm>

namespace pfd::core {

QString AppSettings::settingsFilePath() {
  const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  QDir().mkpath(dir);
  return QDir(dir).filePath(QStringLiteral("app.ini"));
}

AppSettings AppSettings::load() {
  AppSettings out;

  QSettings s(settingsFilePath(), QSettings::IniFormat);

  out.default_download_dir = s.value(QStringLiteral("download/default_dir")).toString().trimmed();

  out.seed_unlimited = s.value(QStringLiteral("seed/unlimited"), false).toBool();
  out.seed_target_ratio = s.value(QStringLiteral("seed/target_ratio"), 1.00).toDouble();
  out.seed_target_ratio = std::clamp(out.seed_target_ratio, 0.10, 100.00);
  out.seed_max_minutes = s.value(QStringLiteral("seed/max_minutes"), 0).toInt();

  out.file_log_enabled = s.value(QStringLiteral("log/file_enabled"), true).toBool();
  out.log_dir = s.value(QStringLiteral("log/dir")).toString().trimmed();
  out.log_level =
      s.value(QStringLiteral("log/level"), QStringLiteral("info")).toString().trimmed().toLower();
  out.log_rotate_max_file_size_bytes =
      s.value(QStringLiteral("log/rotate_max_file_size_bytes"), 0).toLongLong();
  out.log_rotate_max_backup_files =
      s.value(QStringLiteral("log/rotate_max_backup_files"), 3).toInt();
  out.log_rotate_max_backup_files = std::clamp(out.log_rotate_max_backup_files, 1, 50);

  out.download_rate_limit_kib =
      s.value(QStringLiteral("network/download_rate_limit_kib"), 0).toInt();
  out.upload_rate_limit_kib = s.value(QStringLiteral("network/upload_rate_limit_kib"), 0).toInt();
  out.connections_limit = s.value(QStringLiteral("network/connections_limit"), 200).toInt();
  out.per_torrent_connections_limit =
      s.value(QStringLiteral("network/per_torrent_connections_limit"), 0).toInt();
  out.upload_slots_limit = s.value(QStringLiteral("network/upload_slots_limit"), 0).toInt();
  out.per_torrent_upload_slots_limit =
      s.value(QStringLiteral("network/per_torrent_upload_slots_limit"), 0).toInt();
  out.listen_port = s.value(QStringLiteral("network/listen_port"), 0).toInt();
  out.listen_port_forwarding =
      s.value(QStringLiteral("network/listen_port_forwarding"), true).toBool();
  out.active_downloads = s.value(QStringLiteral("queue/active_downloads"), 0).toInt();
  out.active_seeds = s.value(QStringLiteral("queue/active_seeds"), 0).toInt();
  out.active_limit = s.value(QStringLiteral("queue/active_limit"), 0).toInt();
  out.max_active_checking = s.value(QStringLiteral("queue/max_active_checking"), 1).toInt();
  out.auto_manage_prefer_seeds =
      s.value(QStringLiteral("queue/auto_manage_prefer_seeds"), false).toBool();
  out.dont_count_slow_torrents =
      s.value(QStringLiteral("queue/dont_count_slow_torrents"), true).toBool();
  out.slow_torrent_dl_rate_threshold =
      s.value(QStringLiteral("queue/slow_torrent_dl_rate_threshold"), 2).toInt();
  out.slow_torrent_ul_rate_threshold =
      s.value(QStringLiteral("queue/slow_torrent_ul_rate_threshold"), 2).toInt();
  out.slow_torrent_inactive_timer =
      s.value(QStringLiteral("queue/slow_torrent_inactive_timer"), 60).toInt();

  out.enable_dht = s.value(QStringLiteral("protocol/enable_dht"), true).toBool();
  out.enable_upnp = s.value(QStringLiteral("protocol/enable_upnp"), true).toBool();
  out.enable_natpmp = s.value(QStringLiteral("protocol/enable_natpmp"), true).toBool();
  out.enable_lsd = s.value(QStringLiteral("protocol/enable_lsd"), true).toBool();
  out.monitor_port = s.value(QStringLiteral("protocol/monitor_port"), 0).toInt();
  out.builtin_tracker_enabled =
      s.value(QStringLiteral("protocol/builtin_tracker_enabled"), false).toBool();
  out.builtin_tracker_port = s.value(QStringLiteral("protocol/builtin_tracker_port"), 0).toInt();
  out.builtin_tracker_port_forwarding =
      s.value(QStringLiteral("protocol/builtin_tracker_port_forwarding"), false).toBool();
  out.encryption_mode =
      s.value(QStringLiteral("protocol/encryption_mode"), QStringLiteral("enabled"))
          .toString()
          .trimmed()
          .toLower();
  out.ip_filter_enabled = s.value(QStringLiteral("network/ip_filter/enabled"), false).toBool();
  out.ip_filter_path =
      s.value(QStringLiteral("network/ip_filter/path"), QString()).toString().trimmed();
  out.proxy_enabled = s.value(QStringLiteral("network/proxy/enabled"), false).toBool();
  out.proxy_type = s.value(QStringLiteral("network/proxy/type"), QStringLiteral("socks5"))
                       .toString()
                       .trimmed()
                       .toLower();
  out.proxy_host = s.value(QStringLiteral("network/proxy/host"), QString()).toString().trimmed();
  out.proxy_port = s.value(QStringLiteral("network/proxy/port"), 1080).toInt();
  out.proxy_username =
      s.value(QStringLiteral("network/proxy/username"), QString()).toString().trimmed();
  out.proxy_password = s.value(QStringLiteral("network/proxy/password"), QString()).toString();
  out.download_rate_limit_kib = std::clamp(out.download_rate_limit_kib, 0, 1024 * 1024);
  out.upload_rate_limit_kib = std::clamp(out.upload_rate_limit_kib, 0, 1024 * 1024);
  out.connections_limit = std::clamp(out.connections_limit, 0, 20000);
  out.per_torrent_connections_limit = std::clamp(out.per_torrent_connections_limit, 0, 20000);
  out.upload_slots_limit = std::clamp(out.upload_slots_limit, 0, 20000);
  out.per_torrent_upload_slots_limit = std::clamp(out.per_torrent_upload_slots_limit, 0, 20000);
  out.active_downloads = std::clamp(out.active_downloads, 0, 20000);
  out.active_seeds = std::clamp(out.active_seeds, 0, 20000);
  out.active_limit = std::clamp(out.active_limit, 0, 20000);
  out.max_active_checking = std::clamp(out.max_active_checking, 0, 100);
  out.slow_torrent_dl_rate_threshold = std::clamp(out.slow_torrent_dl_rate_threshold, 0, 10240);
  out.slow_torrent_ul_rate_threshold = std::clamp(out.slow_torrent_ul_rate_threshold, 0, 10240);
  out.slow_torrent_inactive_timer = std::clamp(out.slow_torrent_inactive_timer, 0, 3600);
  out.seed_max_minutes = std::clamp(out.seed_max_minutes, 0, 10080);
  out.monitor_port = std::clamp(out.monitor_port, 0, 65535);
  out.listen_port = std::clamp(out.listen_port, 0, 65535);
  out.proxy_port = std::clamp(out.proxy_port, 1, 65535);
  out.builtin_tracker_port = std::clamp(out.builtin_tracker_port, 0, 65535);
  if (out.encryption_mode != QStringLiteral("enabled") &&
      out.encryption_mode != QStringLiteral("forced") &&
      out.encryption_mode != QStringLiteral("disabled")) {
    out.encryption_mode = QStringLiteral("enabled");
  }
  if (out.proxy_type != QStringLiteral("socks5") && out.proxy_type != QStringLiteral("http")) {
    out.proxy_type = QStringLiteral("socks5");
  }
  out.close_behavior = s.value(QStringLiteral("ui/close_behavior"), QStringLiteral("minimize"))
                           .toString()
                           .trimmed()
                           .toLower();
  if (out.close_behavior != QStringLiteral("minimize") &&
      out.close_behavior != QStringLiteral("quit")) {
    out.close_behavior = QStringLiteral("minimize");
  }
  out.start_minimized = s.value(QStringLiteral("ui/start_minimized"), false).toBool();
  out.timed_action = s.value(QStringLiteral("ui/timed_action"), QStringLiteral("none"))
                         .toString()
                         .trimmed()
                         .toLower();
  if (out.timed_action != QStringLiteral("none") &&
      out.timed_action != QStringLiteral("quit_app") &&
      out.timed_action != QStringLiteral("shutdown")) {
    out.timed_action = QStringLiteral("none");
  }
  out.timed_action_delay_minutes =
      s.value(QStringLiteral("ui/timed_action_delay_minutes"), 0).toInt();
  out.timed_action_delay_minutes = std::clamp(out.timed_action_delay_minutes, 0, 10080);
  out.download_complete_action =
      s.value(QStringLiteral("ui/download_complete_action"), QStringLiteral("none"))
          .toString()
          .trimmed()
          .toLower();
  if (out.download_complete_action != QStringLiteral("none") &&
      out.download_complete_action != QStringLiteral("quit_app") &&
      out.download_complete_action != QStringLiteral("suspend") &&
      out.download_complete_action != QStringLiteral("hibernate") &&
      out.download_complete_action != QStringLiteral("poweroff")) {
    out.download_complete_action = QStringLiteral("none");
  }
  if (out.timed_action != QStringLiteral("none") && out.timed_action_delay_minutes > 0) {
    out.download_complete_action = QStringLiteral("none");
  }
  out.http_user_agent =
      s.value(QStringLiteral("http/user_agent"), QStringLiteral("TorrentLink/1.0"))
          .toString()
          .trimmed();
  out.http_accept_language =
      s.value(QStringLiteral("http/accept_language"), QStringLiteral("zh-CN,zh;q=0.9,en;q=0.8"))
          .toString()
          .trimmed();
  out.http_cookie_header = s.value(QStringLiteral("http/cookie_header"), QString()).toString();

  if (out.default_download_dir.isEmpty()) {
    out.default_download_dir = QStandardPaths::writableLocation(QStandardPaths::DownloadLocation);
  }

  if (out.log_dir.isEmpty()) {
    const QString dataDir = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    out.log_dir = QDir(dataDir).filePath(QStringLiteral("logs"));
  }

  return out;
}

void AppSettings::save(const AppSettings& in) {
  QSettings s(settingsFilePath(), QSettings::IniFormat);

  s.setValue(QStringLiteral("download/default_dir"), in.default_download_dir.trimmed());
  s.setValue(QStringLiteral("seed/unlimited"), in.seed_unlimited);
  s.setValue(QStringLiteral("seed/target_ratio"), in.seed_target_ratio);
  s.setValue(QStringLiteral("seed/max_minutes"), in.seed_max_minutes);
  s.setValue(QStringLiteral("log/file_enabled"), in.file_log_enabled);
  s.setValue(QStringLiteral("log/dir"), in.log_dir.trimmed());
  s.setValue(QStringLiteral("log/level"), in.log_level.trimmed().toLower());
  s.setValue(QStringLiteral("log/rotate_max_file_size_bytes"), in.log_rotate_max_file_size_bytes);
  s.setValue(QStringLiteral("log/rotate_max_backup_files"), in.log_rotate_max_backup_files);
  s.setValue(QStringLiteral("network/download_rate_limit_kib"), in.download_rate_limit_kib);
  s.setValue(QStringLiteral("network/upload_rate_limit_kib"), in.upload_rate_limit_kib);
  s.setValue(QStringLiteral("network/connections_limit"), in.connections_limit);
  s.setValue(QStringLiteral("network/per_torrent_connections_limit"),
             in.per_torrent_connections_limit);
  s.setValue(QStringLiteral("network/upload_slots_limit"), in.upload_slots_limit);
  s.setValue(QStringLiteral("network/per_torrent_upload_slots_limit"),
             in.per_torrent_upload_slots_limit);
  s.setValue(QStringLiteral("network/listen_port"), in.listen_port);
  s.setValue(QStringLiteral("network/listen_port_forwarding"), in.listen_port_forwarding);
  s.setValue(QStringLiteral("queue/active_downloads"), in.active_downloads);
  s.setValue(QStringLiteral("queue/active_seeds"), in.active_seeds);
  s.setValue(QStringLiteral("queue/active_limit"), in.active_limit);
  s.setValue(QStringLiteral("queue/max_active_checking"), in.max_active_checking);
  s.setValue(QStringLiteral("queue/auto_manage_prefer_seeds"), in.auto_manage_prefer_seeds);
  s.setValue(QStringLiteral("queue/dont_count_slow_torrents"), in.dont_count_slow_torrents);
  s.setValue(QStringLiteral("queue/slow_torrent_dl_rate_threshold"),
             in.slow_torrent_dl_rate_threshold);
  s.setValue(QStringLiteral("queue/slow_torrent_ul_rate_threshold"),
             in.slow_torrent_ul_rate_threshold);
  s.setValue(QStringLiteral("queue/slow_torrent_inactive_timer"), in.slow_torrent_inactive_timer);
  s.setValue(QStringLiteral("protocol/enable_dht"), in.enable_dht);
  s.setValue(QStringLiteral("protocol/enable_upnp"), in.enable_upnp);
  s.setValue(QStringLiteral("protocol/enable_natpmp"), in.enable_natpmp);
  s.setValue(QStringLiteral("protocol/enable_lsd"), in.enable_lsd);
  s.setValue(QStringLiteral("protocol/monitor_port"), in.monitor_port);
  s.setValue(QStringLiteral("protocol/builtin_tracker_enabled"), in.builtin_tracker_enabled);
  s.setValue(QStringLiteral("protocol/builtin_tracker_port"), in.builtin_tracker_port);
  s.setValue(QStringLiteral("protocol/builtin_tracker_port_forwarding"),
             in.builtin_tracker_port_forwarding);
  s.setValue(QStringLiteral("protocol/encryption_mode"), in.encryption_mode.trimmed().toLower());
  s.setValue(QStringLiteral("network/ip_filter/enabled"), in.ip_filter_enabled);
  s.setValue(QStringLiteral("network/ip_filter/path"), in.ip_filter_path.trimmed());
  s.setValue(QStringLiteral("network/proxy/enabled"), in.proxy_enabled);
  s.setValue(QStringLiteral("network/proxy/type"), in.proxy_type.trimmed().toLower());
  s.setValue(QStringLiteral("network/proxy/host"), in.proxy_host.trimmed());
  s.setValue(QStringLiteral("network/proxy/port"), in.proxy_port);
  s.setValue(QStringLiteral("network/proxy/username"), in.proxy_username.trimmed());
  s.setValue(QStringLiteral("network/proxy/password"), in.proxy_password);
  s.setValue(QStringLiteral("ui/close_behavior"), in.close_behavior.trimmed().toLower());
  s.setValue(QStringLiteral("ui/start_minimized"), in.start_minimized);
  s.setValue(QStringLiteral("ui/timed_action"), in.timed_action.trimmed().toLower());
  s.setValue(QStringLiteral("ui/timed_action_delay_minutes"), in.timed_action_delay_minutes);
  const QString timedAction = in.timed_action.trimmed().toLower();
  const bool timedActionEnabled =
      timedAction != QStringLiteral("none") && in.timed_action_delay_minutes > 0;
  const QString fallbackAction =
      timedActionEnabled ? QStringLiteral("none") : in.download_complete_action.trimmed().toLower();
  s.setValue(QStringLiteral("ui/download_complete_action"), fallbackAction);
  s.setValue(QStringLiteral("http/user_agent"), in.http_user_agent.trimmed());
  s.setValue(QStringLiteral("http/accept_language"), in.http_accept_language.trimmed());
  s.setValue(QStringLiteral("http/cookie_header"), in.http_cookie_header);
}

}  // namespace pfd::core
