#include "ui/state/state_store.h"

#include <QtCore/QDir>
#include <QtCore/QSettings>

#include "core/app_settings.h"

namespace pfd::ui {

namespace {

QSettings settings() {
  const QString path = pfd::core::AppSettings::settingsFilePath();
  return QSettings(QDir::cleanPath(path), QSettings::IniFormat);
}

}  // namespace

MainWindowUiState StateStore::loadMainWindowState() {
  MainWindowUiState out;
  QSettings s = settings();
  s.beginGroup(QStringLiteral("ui/main_window"));
  out.geometry = s.value(QStringLiteral("geometry")).toByteArray();
  out.state = s.value(QStringLiteral("state")).toByteArray();
  out.tab = s.value(QStringLiteral("tab"), 0).toInt();
  out.taskHeaderState = s.value(QStringLiteral("task_header_state")).toByteArray();
  out.sortKey = s.value(QStringLiteral("sort_key"), 0).toInt();
  out.sortOrder = s.value(QStringLiteral("sort_order"), 0).toInt();
  out.filter = s.value(QStringLiteral("filter"), 0).toInt();
  s.endGroup();
  return out;
}

void StateStore::saveMainWindowState(const MainWindowUiState& st) {
  QSettings s = settings();
  s.beginGroup(QStringLiteral("ui/main_window"));
  s.setValue(QStringLiteral("geometry"), st.geometry);
  s.setValue(QStringLiteral("state"), st.state);
  s.setValue(QStringLiteral("tab"), st.tab);
  s.setValue(QStringLiteral("task_header_state"), st.taskHeaderState);
  s.setValue(QStringLiteral("sort_key"), st.sortKey);
  s.setValue(QStringLiteral("sort_order"), st.sortOrder);
  s.setValue(QStringLiteral("filter"), st.filter);
  s.endGroup();
}

LogCenterUiState StateStore::loadLogCenterState() {
  LogCenterUiState out;
  QSettings s = settings();
  s.beginGroup(QStringLiteral("ui/log_center"));
  out.geometry = s.value(QStringLiteral("geometry")).toByteArray();
  out.level = s.value(QStringLiteral("level"), QStringLiteral("all")).toString();
  out.keyword = s.value(QStringLiteral("keyword"), QString()).toString();
  s.endGroup();
  return out;
}

void StateStore::saveLogCenterState(const LogCenterUiState& st) {
  QSettings s = settings();
  s.beginGroup(QStringLiteral("ui/log_center"));
  s.setValue(QStringLiteral("geometry"), st.geometry);
  s.setValue(QStringLiteral("level"), st.level);
  s.setValue(QStringLiteral("keyword"), st.keyword);
  s.endGroup();
}

}  // namespace pfd::ui
