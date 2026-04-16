#ifndef PFD_UI_SYSTEM_TRAY_H
#define PFD_UI_SYSTEM_TRAY_H

#include <QtCore/QObject>

class QAction;
class QMenu;
class QSystemTrayIcon;

namespace pfd::ui {

class SystemTray : public QObject {
  Q_OBJECT

public:
  explicit SystemTray(QObject* parent = nullptr);

  void show();
  void showNotification(const QString& title, const QString& message);

Q_SIGNALS:
  void showWindowRequested();
  void hideWindowRequested();
  void pauseAllRequested();
  void resumeAllRequested();
  void quitRequested();

private:
  void buildTrayMenu();

  QSystemTrayIcon* trayIcon_{nullptr};
  QMenu* trayMenu_{nullptr};
  QAction* showAction_{nullptr};
  QAction* pauseAllAction_{nullptr};
  QAction* resumeAllAction_{nullptr};
  QAction* quitAction_{nullptr};
};

}  // namespace pfd::ui

#endif  // PFD_UI_SYSTEM_TRAY_H
