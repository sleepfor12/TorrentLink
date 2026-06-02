#include <QtCore/QDir>
#include <QtCore/QSignalBlocker>
#include <QtCore/QStringList>
#include <QtGui/QActionGroup>
#include <QtWidgets/QAbstractItemView>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenu>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>

#include "core/config_service.h"
#include "ui/app_theme.h"
#include "ui/main_window.h"
#include "ui/pages/detail/speed_chart_page.h"
#include "ui/pages/detail/tracker_detail_page.h"
#include "ui/pages/transfer_page.h"
#include "ui/rss/rss_module_page.h"
#include "ui/widgets/bottom_status_bar.h"

namespace pfd::ui {

void MainWindow::setupMenuBar() {
  auto* fileMenu = menuBar()->addMenu(QStringLiteral("文件"));
  openTorrentFileAction_ = fileMenu->addAction(QStringLiteral("打开 Torrent 文件"));
  openTorrentLinksAction_ = fileMenu->addAction(QStringLiteral("打开 Torrent 链接"));
  fileMenu->addSeparator();
  exitAction_ = fileMenu->addAction(QStringLiteral("退出"));
  auto* toolsMenu = menuBar()->addMenu(QStringLiteral("工具"));
  preferencesAction_ = toolsMenu->addAction(QStringLiteral("首选项"));
  themeMenu_ = toolsMenu->addMenu(QStringLiteral("界面主题"));
  themeActionGroup_ = new QActionGroup(this);
  themeActionGroup_->setExclusive(true);
  themeLightAction_ = themeMenu_->addAction(QStringLiteral("浅色"));
  themeLightAction_->setCheckable(true);
  themeDarkAction_ = themeMenu_->addAction(QStringLiteral("深色"));
  themeDarkAction_->setCheckable(true);
  themeSystemAction_ = themeMenu_->addAction(QStringLiteral("跟随系统"));
  themeSystemAction_->setCheckable(true);
#if QT_VERSION < QT_VERSION_CHECK(6, 5, 0)
  themeSystemAction_->setToolTip(
      QStringLiteral("当前 Qt 版本低于 6.5 时，“跟随系统”与浅色效果相同。"));
#endif
  themeActionGroup_->addAction(themeLightAction_);
  themeActionGroup_->addAction(themeDarkAction_);
  themeActionGroup_->addAction(themeSystemAction_);
  toolsMenu->addSeparator();
  logCenterAction_ = toolsMenu->addAction(QStringLiteral("日志中心"));
  toolsMenu->addSeparator();
  createTorrentAction_ = toolsMenu->addAction(QStringLiteral("生成 Torrent"));
  manageCookiesAction_ = toolsMenu->addAction(QStringLiteral("管理 Cookies"));
  auto* viewMenu = menuBar()->addMenu(QStringLiteral("视图"));
  showLogAction_ = viewMenu->addAction(QStringLiteral("显示日志面板"));
  showBottomStatusBarAction_ = viewMenu->addAction(QStringLiteral("显示底部状态栏"));
  showBottomStatusBarAction_->setCheckable(true);
  showBottomStatusBarAction_->setChecked(true);
  showTransferDetailPanelAction_ = viewMenu->addAction(QStringLiteral("显示传输页任务详情"));
  showTransferDetailPanelAction_->setCheckable(true);
  showTransferDetailPanelAction_->setChecked(true);
  viewMenu->addSeparator();
  refreshListAction_ = viewMenu->addAction(QStringLiteral("刷新任务列表"));
  auto* helpMenu = menuBar()->addMenu(QStringLiteral("帮助"));
  helpAction_ = helpMenu->addAction(QStringLiteral("使用说明"));
  aboutAction_ = helpMenu->addAction(QStringLiteral("关于"));
}

void MainWindow::applyTheme() {
  UiTheme::applyApplicationFont();
  const auto st = pfd::core::ConfigService::loadAppSettings();
  const EffectiveUiTheme t = UiTheme::resolveEffectiveTheme(st.ui_theme);
  setStyleSheet(UiTheme::mainWindowStyleSheet(t));
  for (QAbstractItemView* view : findChildren<QAbstractItemView*>()) {
    if (view != nullptr) {
      UiTheme::applyItemViewSelectionPalette(view, t);
    }
  }
  for (SpeedChartPage* chart : findChildren<SpeedChartPage*>()) {
    if (chart != nullptr) {
      chart->syncChartTheme();
    }
  }
  for (TrackerDetailPage* tracker : findChildren<TrackerDetailPage*>()) {
    if (tracker != nullptr) {
      tracker->syncTheme();
    }
  }
  if (transferPage_ != nullptr) {
    transferPage_->syncDetailTheme();
  }
}

void MainWindow::syncThemeMenuChecks() {
  if (themeLightAction_ == nullptr || themeDarkAction_ == nullptr ||
      themeSystemAction_ == nullptr) {
    return;
  }
  const QString pref = pfd::core::ConfigService::loadAppSettings().ui_theme.trimmed().toLower();
  const QSignalBlocker b0(themeLightAction_);
  const QSignalBlocker b1(themeDarkAction_);
  const QSignalBlocker b2(themeSystemAction_);
  if (pref == QStringLiteral("dark")) {
    themeDarkAction_->setChecked(true);
  } else if (pref == QStringLiteral("system")) {
    themeSystemAction_->setChecked(true);
  } else {
    themeLightAction_->setChecked(true);
  }
}

void MainWindow::buildLayout() {
  auto* central = new QWidget(this);
  auto* outer = new QVBoxLayout(central);
  outer->setContentsMargins(16, 16, 16, 16);
  outer->setSpacing(10);

  tabs_ = new QTabWidget(central);
  tabs_->setDocumentMode(true);
  tabs_->setMovable(false);
  tabs_->setTabsClosable(false);

  // --- Tab 1: 传输 ---
  transferPage_ = new TransferPage(tabs_);
  taskTable_ = transferPage_->taskTable();

  tabs_->addTab(transferPage_, QStringLiteral("传输"));

  // --- Tab 2: RSS 订阅（模块化入口） ---
  rssModulePage_ = new pfd::ui::rss::RssModulePage(tabs_);
  tabs_->addTab(rssModulePage_, QStringLiteral("RSS 订阅"));

  // 搜索页暂时隐藏：保留对象指针为空，避免外部调用空指针时崩溃。
  searchTab_ = nullptr;

  outer->addWidget(tabs_, 1);

  bottomStatus_ = new BottomStatusBar(central);
  outer->addWidget(bottomStatus_, 0);
  setCentralWidget(central);
}

}  // namespace pfd::ui
