#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtWidgets/QApplication>

#include <atomic>
#include <chrono>
#include <future>
#include <gtest/gtest.h>

#include "app/exit_coordinator.h"

TEST(ExitCoordinatorTest, BeginShutdownSetsFlag) {
  pfd::app::ExitCoordinator coordinator;
  std::atomic<bool> shuttingDown{false};
  coordinator.beginShutdown(shuttingDown);
  EXPECT_TRUE(shuttingDown.load());
}

TEST(ExitCoordinatorTest, WaitBackgroundTasksConsumesReadyFutures) {
  pfd::app::ExitCoordinator coordinator;
  std::promise<void> p1;
  std::promise<void> p2;
  std::future<void> f1 = p1.get_future();
  std::future<void> status = p2.get_future();
  std::vector<std::future<void>> tasks;
  tasks.push_back(std::move(f1));
  p1.set_value();
  p2.set_value();

  coordinator.waitBackgroundTasks(tasks, status, std::chrono::milliseconds(5));

  ASSERT_TRUE(tasks.front().valid());
  EXPECT_TRUE(status.valid());
  EXPECT_EQ(tasks.front().wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
  EXPECT_EQ(status.wait_for(std::chrono::milliseconds(0)), std::future_status::ready);
}
