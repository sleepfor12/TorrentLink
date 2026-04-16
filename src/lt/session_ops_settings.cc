#include <libtorrent/session.hpp>
#include <libtorrent/settings_pack.hpp>

#include <algorithm>

#include "core/logger.h"
#include "lt/session_ids.h"
#include "lt/session_ops.h"

namespace pfd::lt::session_ops {

bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::ApplyRuntimeSettingsCmd& c) {
  libtorrent::settings_pack pack;
  if (c.download_rate_limit_kib > 0) {
    pack.set_int(libtorrent::settings_pack::download_rate_limit, c.download_rate_limit_kib * 1024);
  } else if (c.download_rate_limit_kib == 0) {
    pack.set_int(libtorrent::settings_pack::download_rate_limit, 0);
  }
  if (c.upload_rate_limit_kib > 0) {
    pack.set_int(libtorrent::settings_pack::upload_rate_limit, c.upload_rate_limit_kib * 1024);
  } else if (c.upload_rate_limit_kib == 0) {
    pack.set_int(libtorrent::settings_pack::upload_rate_limit, 0);
  }
  if (c.connections_limit > 0) {
    pack.set_int(libtorrent::settings_pack::connections_limit, c.connections_limit);
  } else if (c.connections_limit == 0) {
    pack.set_int(libtorrent::settings_pack::connections_limit, 0);
  }
  if (c.upload_slots_limit > 0) {
    pack.set_int(libtorrent::settings_pack::unchoke_slots_limit, c.upload_slots_limit);
  } else if (c.upload_slots_limit == 0) {
    pack.set_int(libtorrent::settings_pack::unchoke_slots_limit, -1);
  }
  pack.set_str(
      libtorrent::settings_pack::listen_interfaces,
      QStringLiteral("0.0.0.0:%1,[::]:%1").arg(std::clamp(c.listen_port, 0, 65535)).toStdString());

  if (c.active_downloads > 0) {
    pack.set_int(libtorrent::settings_pack::active_downloads, c.active_downloads);
  } else if (c.active_downloads == 0) {
    pack.set_int(libtorrent::settings_pack::active_downloads, 0);
  }
  if (c.active_seeds > 0) {
    pack.set_int(libtorrent::settings_pack::active_seeds, c.active_seeds);
  } else if (c.active_seeds == 0) {
    pack.set_int(libtorrent::settings_pack::active_seeds, 0);
  }
  if (c.active_limit > 0) {
    pack.set_int(libtorrent::settings_pack::active_limit, c.active_limit);
  } else if (c.active_limit == 0) {
    pack.set_int(libtorrent::settings_pack::active_limit, 0);
  }
  if (c.max_active_checking > 0) {
    pack.set_int(libtorrent::settings_pack::active_checking, c.max_active_checking);
  }
  pack.set_bool(libtorrent::settings_pack::auto_manage_prefer_seeds, c.auto_manage_prefer_seeds);
  pack.set_bool(libtorrent::settings_pack::dont_count_slow_torrents, c.dont_count_slow_torrents);
  if (c.slow_torrent_dl_rate_threshold > 0) {
    pack.set_int(libtorrent::settings_pack::inactive_down_rate,
                 c.slow_torrent_dl_rate_threshold * 1024);
  }
  if (c.slow_torrent_ul_rate_threshold > 0) {
    pack.set_int(libtorrent::settings_pack::inactive_up_rate,
                 c.slow_torrent_ul_rate_threshold * 1024);
  }
  if (c.slow_torrent_inactive_timer > 0) {
    pack.set_int(libtorrent::settings_pack::auto_manage_startup, c.slow_torrent_inactive_timer);
  }

  pack.set_bool(libtorrent::settings_pack::enable_dht, c.enable_dht);
  const bool enablePortForwarding = c.listen_port_forwarding;
  pack.set_bool(libtorrent::settings_pack::enable_upnp, c.enable_upnp && enablePortForwarding);
  pack.set_bool(libtorrent::settings_pack::enable_natpmp, c.enable_natpmp && enablePortForwarding);
  pack.set_bool(libtorrent::settings_pack::enable_lsd, c.enable_lsd);
  if (c.proxy_enabled && !c.proxy_host.trimmed().isEmpty()) {
    const QString type = c.proxy_type.trimmed().toLower();
    int proxyType = libtorrent::settings_pack::socks5;
    if (type == QStringLiteral("http")) {
      proxyType = c.proxy_username.trimmed().isEmpty() && c.proxy_password.isEmpty()
                      ? libtorrent::settings_pack::http
                      : libtorrent::settings_pack::http_pw;
    } else {
      proxyType = c.proxy_username.trimmed().isEmpty() && c.proxy_password.isEmpty()
                      ? libtorrent::settings_pack::socks5
                      : libtorrent::settings_pack::socks5_pw;
    }
    pack.set_int(libtorrent::settings_pack::proxy_type, proxyType);
    pack.set_str(libtorrent::settings_pack::proxy_hostname, c.proxy_host.trimmed().toStdString());
    pack.set_int(libtorrent::settings_pack::proxy_port, std::clamp(c.proxy_port, 1, 65535));
    pack.set_str(libtorrent::settings_pack::proxy_username,
                 c.proxy_username.trimmed().toStdString());
    pack.set_str(libtorrent::settings_pack::proxy_password, c.proxy_password.toStdString());
    pack.set_bool(libtorrent::settings_pack::proxy_peer_connections, true);
    pack.set_bool(libtorrent::settings_pack::proxy_hostnames, true);
  } else {
    pack.set_int(libtorrent::settings_pack::proxy_type, libtorrent::settings_pack::none);
  }

  const QString mode = c.encryption_mode.trimmed().toLower();
  if (mode == QStringLiteral("forced")) {
    pack.set_int(libtorrent::settings_pack::in_enc_policy, libtorrent::settings_pack::pe_forced);
    pack.set_int(libtorrent::settings_pack::out_enc_policy, libtorrent::settings_pack::pe_forced);
    pack.set_int(libtorrent::settings_pack::allowed_enc_level, libtorrent::settings_pack::pe_both);
  } else if (mode == QStringLiteral("disabled")) {
    pack.set_int(libtorrent::settings_pack::in_enc_policy, libtorrent::settings_pack::pe_disabled);
    pack.set_int(libtorrent::settings_pack::out_enc_policy, libtorrent::settings_pack::pe_disabled);
    pack.set_int(libtorrent::settings_pack::allowed_enc_level,
                 libtorrent::settings_pack::pe_plaintext);
  } else {
    pack.set_int(libtorrent::settings_pack::in_enc_policy, libtorrent::settings_pack::pe_enabled);
    pack.set_int(libtorrent::settings_pack::out_enc_policy, libtorrent::settings_pack::pe_enabled);
    pack.set_int(libtorrent::settings_pack::allowed_enc_level, libtorrent::settings_pack::pe_both);
  }

  ses.apply_settings(pack);
  if (c.per_torrent_upload_slots_limit >= 0) {
    // Apply per-torrent upload slots for existing handles best-effort.
    for (auto& [_, h] : ctx.handlesByTaskId) {
      if (h.is_valid()) {
        h.set_max_uploads(c.per_torrent_upload_slots_limit);
      }
    }
  }
  LOG_INFO(QStringLiteral(
               "[lt.worker] Runtime settings applied dl=%1KiB/s ul=%2KiB/s conn=%3 active_dl=%4 "
               "active_seed=%5 active=%6 up_slots=%7 pt_up_slots=%8 listen=%9 dht=%10 upnp=%11 "
               "natpmp=%12 lsd=%13 proxy=%14 enc=%15")
               .arg(c.download_rate_limit_kib)
               .arg(c.upload_rate_limit_kib)
               .arg(c.connections_limit)
               .arg(c.active_downloads)
               .arg(c.active_seeds)
               .arg(c.active_limit)
               .arg(c.upload_slots_limit)
               .arg(c.per_torrent_upload_slots_limit)
               .arg(c.listen_port)
               .arg(c.enable_dht ? QStringLiteral("on") : QStringLiteral("off"))
               .arg(c.enable_upnp ? QStringLiteral("on") : QStringLiteral("off"))
               .arg(c.enable_natpmp ? QStringLiteral("on") : QStringLiteral("off"))
               .arg(c.enable_lsd ? QStringLiteral("on") : QStringLiteral("off"))
               .arg(c.proxy_enabled ? QStringLiteral("on") : QStringLiteral("off"))
               .arg(mode));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::SetDefaultPerTorrentConnectionsLimitCmd& c) {
  ctx.defaultPerTorrentConnectionsLimit = std::clamp(c.limit, 0, 20000);
  for (auto& [_, h] : ctx.handlesByTaskId) {
    if (h.is_valid()) {
      h.set_max_connections(ctx.defaultPerTorrentConnectionsLimit);
    }
  }
  LOG_INFO(QStringLiteral("[lt.worker] Default per-torrent connections limit=%1")
               .arg(ctx.defaultPerTorrentConnectionsLimit));
  return true;
}

bool handleOne(libtorrent::session&, Context& ctx,
               const session_cmds::SetTaskConnectionsLimitCmd& c) {
  const QString key = session_ids::taskIdKey(c.taskId);
  const int v = std::clamp(c.limit, 0, 20000);
  ctx.perTaskConnectionsLimit[key] = v;
  const auto it = ctx.handlesByTaskId.find(key);
  if (it != ctx.handlesByTaskId.end() && it->second.is_valid()) {
    it->second.set_max_connections(v);
    LOG_INFO(QStringLiteral("[lt.worker] Task connections limit applied taskId=%1 limit=%2")
                 .arg(c.taskId.toString())
                 .arg(v));
  } else {
    LOG_INFO(QStringLiteral("[lt.worker] Task connections limit queued taskId=%1 limit=%2")
                 .arg(c.taskId.toString())
                 .arg(v));
  }
  return true;
}

}  // namespace pfd::lt::session_ops
