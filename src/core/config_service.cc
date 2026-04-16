#include "core/config_service.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

#include <algorithm>

#include "core/app_settings.h"
#include "core/logger.h"

namespace pfd::core {
namespace {

QString settingsStatusToString(QSettings::Status status) {
  switch (status) {
    case QSettings::NoError:
      return QStringLiteral("NoError");
    case QSettings::AccessError:
      return QStringLiteral("AccessError");
    case QSettings::FormatError:
      return QStringLiteral("FormatError");
  }
  return QStringLiteral("Unknown");
}

}  // namespace

QStringList ConfigService::normalizeTrackers(const QStringList& trackers) {
  QStringList out;
  for (const auto& t : trackers) {
    const QString trimmed = t.trimmed();
    if (!trimmed.isEmpty() && !out.contains(trimmed)) {
      out.push_back(trimmed);
    }
  }
  return out;
}

AppSettings ConfigService::loadAppSettings() {
  return AppSettings::load();
}

SaveResult ConfigService::saveAppSettings(const AppSettings& settings) {
  AppSettings::save(settings);
  QSettings probe(AppSettings::settingsFilePath(), QSettings::IniFormat);
  probe.sync();
  if (probe.status() != QSettings::NoError) {
    const QString msg = QStringLiteral("save app settings failed status=%1")
                            .arg(settingsStatusToString(probe.status()));
    LOG_ERROR(QStringLiteral("[config] %1").arg(msg));
    return SaveResult{false, msg};
  }
  return SaveResult{};
}

CombinedSettings ConfigService::loadCombinedSettings() {
  CombinedSettings out;
  out.app = loadAppSettings();
  out.controller = loadControllerSettings();
  return out;
}

SaveResult ConfigService::saveCombinedSettings(const CombinedSettings& settings) {
  const auto app_result = saveAppSettings(settings.app);
  const auto controller_result = saveControllerSettings(settings.controller);
  if (!app_result.ok && !controller_result.ok) {
    return SaveResult{false,
                      QStringLiteral("%1; %2").arg(app_result.message, controller_result.message)};
  }
  if (!app_result.ok) {
    return app_result;
  }
  if (!controller_result.ok) {
    return controller_result;
  }
  return SaveResult{};
}

ControllerSettings ConfigService::loadControllerSettings() {
  ControllerSettings out;
  QSettings settings(AppSettings::settingsFilePath(), QSettings::IniFormat);
  out.auto_apply_default_trackers =
      settings.value(QStringLiteral("trackers/auto_apply"), false).toBool();
  out.default_trackers =
      normalizeTrackers(settings.value(QStringLiteral("trackers/default_list")).toStringList());
  out.magnet_max_inflight =
      std::clamp(settings.value(QStringLiteral("ui/magnet/max_inflight"), 8).toInt(), 5, 10);
  out.bottom_status_enabled = settings.value(QStringLiteral("ui/status/enabled"), true).toBool();
  out.bottom_status_refresh_ms =
      std::clamp(settings.value(QStringLiteral("ui/status/refresh_ms"), 1000).toInt(), 500, 5000);
  out.task_autosave_ms =
      std::clamp(settings.value(QStringLiteral("task/autosave_ms"), 5000).toInt(), 2000, 60000);
  out.restore_start_paused =
      settings.value(QStringLiteral("task/restore_start_paused"), true).toBool();
  if (settings.status() != QSettings::NoError) {
    LOG_WARN(QStringLiteral("[config] load controller settings status=%1")
                 .arg(settingsStatusToString(settings.status())));
  }
  return out;
}

SaveResult ConfigService::saveControllerSettings(const ControllerSettings& in) {
  const QString settingsPath = AppSettings::settingsFilePath();
  QDir().mkpath(QFileInfo(settingsPath).absolutePath());
  QSettings settings(settingsPath, QSettings::IniFormat);
  settings.setValue(QStringLiteral("trackers/auto_apply"), in.auto_apply_default_trackers);
  settings.setValue(QStringLiteral("trackers/default_list"),
                    normalizeTrackers(in.default_trackers));
  settings.setValue(QStringLiteral("ui/magnet/max_inflight"), in.magnet_max_inflight);
  settings.setValue(QStringLiteral("ui/status/enabled"), in.bottom_status_enabled);
  settings.setValue(QStringLiteral("ui/status/refresh_ms"), in.bottom_status_refresh_ms);
  settings.setValue(QStringLiteral("task/autosave_ms"), in.task_autosave_ms);
  settings.setValue(QStringLiteral("task/restore_start_paused"), in.restore_start_paused);
  settings.sync();
  if (settings.status() != QSettings::NoError) {
    const QString msg = QStringLiteral("save controller settings failed path=%1 status=%2")
                            .arg(settingsPath, settingsStatusToString(settings.status()));
    LOG_ERROR(QStringLiteral("[config] %1").arg(msg));
    return SaveResult{false, msg};
  }
  return SaveResult{};
}

}  // namespace pfd::core
