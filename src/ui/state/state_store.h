#ifndef PFD_UI_STATE_STATE_STORE_H
#define PFD_UI_STATE_STATE_STORE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

namespace pfd::ui {

struct MainWindowUiState {
  QByteArray geometry;
  QByteArray state;
  int tab{0};
  QByteArray taskHeaderState;
  int sortKey{0};
  int sortOrder{0};
  int filter{0};
};

struct LogCenterUiState {
  QByteArray geometry;
  QString level;  // "all"/"debug"/...
  QString keyword;
};

class StateStore final {
public:
  static MainWindowUiState loadMainWindowState();
  static void saveMainWindowState(const MainWindowUiState& st);

  static LogCenterUiState loadLogCenterState();
  static void saveLogCenterState(const LogCenterUiState& st);
};

}  // namespace pfd::ui

#endif  // PFD_UI_STATE_STATE_STORE_H
