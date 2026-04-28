#include <QtCore/QByteArray>
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtWidgets/QPushButton>
#include <QtWidgets/QTabWidget>

#include <gtest/gtest.h>

#include "ui/main_window.h"

namespace {

QTabWidget* findTabs(pfd::ui::MainWindow& w) {
  return w.findChild<QTabWidget*>();
}

}  // namespace

TEST(UiMainWindowTest, HidesSearchTabByDefault) {
  pfd::ui::MainWindow w;
  QTabWidget* tabs = findTabs(w);
  ASSERT_NE(tabs, nullptr);
  bool foundSearch = false;
  for (int i = 0; i < tabs->count(); ++i) {
    if (tabs->tabText(i).contains(QStringLiteral("搜索"))) {
      foundSearch = true;
      break;
    }
  }
  EXPECT_FALSE(foundSearch);
}

TEST(UiMainWindowTest, DefaultsToTransferTabOnStartup) {
  pfd::ui::MainWindow w;
  QTabWidget* tabs = findTabs(w);
  ASSERT_NE(tabs, nullptr);
  ASSERT_GT(tabs->count(), 0);
  EXPECT_EQ(tabs->currentIndex(), 0);
  EXPECT_EQ(tabs->tabText(0), QStringLiteral("传输"));
}

TEST(UiMainWindowTest, HidesHttpSourceDetailTabButton) {
  pfd::ui::MainWindow w;
  const auto buttons = w.findChildren<QPushButton*>();
  bool foundHttpSourceButton = false;
  for (QPushButton* btn : buttons) {
    if (btn == nullptr) {
      continue;
    }
    if (btn->text().contains(QStringLiteral("HTTP源"))) {
      foundHttpSourceButton = true;
      EXPECT_FALSE(btn->isVisible());
    }
  }
  EXPECT_TRUE(foundHttpSourceButton);
}
