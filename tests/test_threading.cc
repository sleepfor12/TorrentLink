#include <QtCore/QCoreApplication>
#include <QtCore/QObject>
#include <QtCore/QThread>
#include <QtCore/QTimer>

#include <atomic>
#include <gtest/gtest.h>

#include "base/threading.h"

namespace {

using pfd::base::runOn;

TEST(Threading, RunOnNullContextReturnsFalse) {
  EXPECT_FALSE(runOn(nullptr, []() {}));
}

TEST(Threading, RunOnSameThreadExecutes) {
  QObject obj;
  std::atomic<bool> executed{false};
  EXPECT_TRUE(runOn(&obj, [&executed]() { executed = true; }));

  QTimer::singleShot(10, QCoreApplication::instance(), &QCoreApplication::quit);
  QCoreApplication::instance()->exec();

  EXPECT_TRUE(executed);
}

TEST(Threading, RunOnTargetThreadExecutesInTargetThread) {
  QThread worker;
  worker.start();

  QObject obj;
  obj.moveToThread(&worker);

  std::atomic<bool> executed{false};
  QThread* executedThread = nullptr;
  EXPECT_TRUE(runOn(&obj, [&executed, &executedThread, &worker]() {
    executed = true;
    executedThread = QThread::currentThread();
    worker.quit();  // 在目标线程执行完毕后再退出
  }));

  worker.wait();

  EXPECT_TRUE(executed);
  EXPECT_EQ(executedThread, &worker);
}

}  // namespace
