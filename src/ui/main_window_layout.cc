#include <QtCore/QDir>
#include <QtCore/QStringList>
#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QMenuBar>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>
#include <QtWidgets/QTableWidget>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QWidget>

#include "ui/main_window.h"
#include "ui/pages/transfer_page.h"
#include "ui/rss/rss_module_page.h"
#include "ui/search_tab.h"
#include "ui/widgets/bottom_status_bar.h"

namespace pfd::ui {

void MainWindow::setupMenuBar() {
  auto* fileMenu = menuBar()->addMenu(QStringLiteral("文件"));
  openTorrentFileAction_ = fileMenu->addAction(QStringLiteral("打开 Torrent 文件"));
  openTorrentLinksAction_ = fileMenu->addAction(QStringLiteral("打开 Torrent 链接"));
  fileMenu->addSeparator();
  exitAction_ = fileMenu->addAction(QStringLiteral("退出"));
  auto* toolsMenu = menuBar()->addMenu(QStringLiteral("工具"));
  downloadSettingsAction_ = toolsMenu->addAction(QStringLiteral("下载设置"));
  logCenterAction_ = toolsMenu->addAction(QStringLiteral("日志中心"));
  auto* viewMenu = menuBar()->addMenu(QStringLiteral("视图"));
  showLogAction_ = viewMenu->addAction(QStringLiteral("显示日志面板"));
  refreshListAction_ = viewMenu->addAction(QStringLiteral("刷新任务列表"));
  auto* helpMenu = menuBar()->addMenu(QStringLiteral("帮助"));
  helpAction_ = helpMenu->addAction(QStringLiteral("使用说明"));
  aboutAction_ = helpMenu->addAction(QStringLiteral("关于"));
  auto* settingsMenu = menuBar()->addMenu(QStringLiteral("设置"));
  preferencesAction_ = settingsMenu->addAction(QStringLiteral("首选项"));
  themeAction_ = settingsMenu->addAction(QStringLiteral("界面主题"));
}

void MainWindow::applyTheme() {
  setStyleSheet(QStringLiteral(
      "QMainWindow{background:#f5f7fb;}"
      "QTabWidget::pane{border:0;background:transparent;}"
      "QTabBar::tab{background:#ffffff;border:1px solid #e7ebf3;border-bottom:0;"
      "padding:10px 14px;margin-right:6px;border-top-left-radius:10px;border-top-right-radius:10px;"
      "color:#41516d;font-weight:700;}"
      "QTabBar::tab:selected{background:#ffffff;color:#1677ff;border-color:#d6e3ff;}"
      "QTabBar::tab:!selected{background:#f8fbff;}"
      "QWidget#TopBar{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QWidget#SideBar{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QLineEdit{background:#f9fbff;border:1px solid #d8e2f0;border-radius:10px;"
      "padding:8px 12px;color:#1f2937;}"
      "QLineEdit:focus{border:1px solid #409eff;}"
      "QPushButton{background:#eef3ff;color:#2b3a55;border:1px solid #d6e3ff;border-radius:10px;"
      "padding:8px 14px;font-weight:600;}"
      "QPushButton:hover{background:#e4edff;}"
      "QPushButton#PrimaryButton{background:#1677ff;color:#ffffff;border:1px solid #1677ff;}"
      "QPushButton#PrimaryButton:hover{background:#3d8bff;}"
      "QPushButton#FilterButton{background:#f5f8ff;color:#41516d;border:1px solid "
      "#d9e5ff;border-radius:14px;}"
      "QPushButton#FilterButton:checked{background:#1677ff;color:#ffffff;border:1px solid #1677ff;}"
      "QWidget#StatCard{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QWidget#BottomStatusBar{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;}"
      "QLabel#BottomStatusLabel{color:#41516d;font-size:12px;font-weight:600;}"
      "QLabel#StatTitle{color:#6b7280;font-size:12px;}"
      "QLabel#StatValue{color:#1f2937;font-size:22px;font-weight:700;}"
      "QTableWidget{background:#ffffff;border:1px solid #e7ebf3;border-radius:12px;"
      "gridline-color:#eff3f9;selection-background-color:#2f6fed;selection-color:#ffffff;}"
      "QTableWidget::item:selected{color:#ffffff;}"
      "QHeaderView::section{background:#f8fbff;color:#50607a;border:none;border-bottom:1px solid "
      "#e7ebf3;"
      "padding:8px;font-weight:600;}"
      ""));
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

  // --- Tab 3: 搜索 ---
  searchTab_ = new SearchTab(tabs_);
  tabs_->addTab(searchTab_, QStringLiteral("搜索"));

  outer->addWidget(tabs_, 1);

  bottomStatus_ = new BottomStatusBar(central);
  outer->addWidget(bottomStatus_, 0);
  setCentralWidget(central);
}

}  // namespace pfd::ui
