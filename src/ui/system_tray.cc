#include "ui/system_tray.h"

#include <QtGui/QAction>
#include <QtWidgets/QApplication>
#include <QtWidgets/QMenu>
#include <QtWidgets/QStyle>
#include <QtWidgets/QSystemTrayIcon>

namespace pfd::ui {

SystemTray::SystemTray(QObject* parent) : QObject(parent) {
  trayIcon_ = new QSystemTrayIcon(this);
  trayIcon_->setIcon(QApplication::style()->standardIcon(QStyle::SP_ArrowDown));
  trayIcon_->setToolTip(QStringLiteral("P2P 下载器"));
  buildTrayMenu();

  connect(trayIcon_, &QSystemTrayIcon::activated, this,
          [this](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
              emit showWindowRequested();
            }
          });
}

void SystemTray::buildTrayMenu() {
  trayMenu_ = new QMenu();
  showAction_ = trayMenu_->addAction(QStringLiteral("显示主窗口"));
  connect(showAction_, &QAction::triggered, this, &SystemTray::showWindowRequested);

  trayMenu_->addSeparator();

  pauseAllAction_ = trayMenu_->addAction(QStringLiteral("暂停全部"));
  connect(pauseAllAction_, &QAction::triggered, this, &SystemTray::pauseAllRequested);

  resumeAllAction_ = trayMenu_->addAction(QStringLiteral("继续全部"));
  connect(resumeAllAction_, &QAction::triggered, this, &SystemTray::resumeAllRequested);

  trayMenu_->addSeparator();

  quitAction_ = trayMenu_->addAction(QStringLiteral("退出"));
  connect(quitAction_, &QAction::triggered, this, &SystemTray::quitRequested);

  trayIcon_->setContextMenu(trayMenu_);
}

void SystemTray::show() {
  trayIcon_->show();
}

void SystemTray::showNotification(const QString& title, const QString& message) {
  if (trayIcon_ != nullptr && trayIcon_->isVisible()) {
    trayIcon_->showMessage(title, message, QSystemTrayIcon::Information, 5000);
  }
}

}  // namespace pfd::ui
