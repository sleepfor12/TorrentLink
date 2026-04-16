#ifndef PFD_CORE_CONFIG_SERVICE_H
#define PFD_CORE_CONFIG_SERVICE_H

#include <QtCore/QStringList>

#include "core/app_settings.h"

namespace pfd::core {

struct ControllerSettings {
  bool auto_apply_default_trackers{false};
  QStringList default_trackers;
  int magnet_max_inflight{8};
  bool bottom_status_enabled{true};
  int bottom_status_refresh_ms{1000};
  int task_autosave_ms{5000};
  bool restore_start_paused{true};
};

struct CombinedSettings {
  AppSettings app;
  ControllerSettings controller;
};

struct SaveResult {
  bool ok{true};
  QString message;
};

class ConfigService {
public:
  static QStringList normalizeTrackers(const QStringList& trackers);
  static AppSettings loadAppSettings();
  static SaveResult saveAppSettings(const AppSettings& settings);
  static ControllerSettings loadControllerSettings();
  static SaveResult saveControllerSettings(const ControllerSettings& settings);
  static CombinedSettings loadCombinedSettings();
  static SaveResult saveCombinedSettings(const CombinedSettings& settings);
};

}  // namespace pfd::core

#endif  // PFD_CORE_CONFIG_SERVICE_H
