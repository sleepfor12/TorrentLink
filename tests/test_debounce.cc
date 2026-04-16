#include <QtCore/QCoreApplication>
#include <QtCore/QTimer>

#include <gtest/gtest.h>

#include "base/debounce.h"

namespace {

TEST(Debounce, CancelPreventsExecution) {
  QObject parent;
  pfd::base::Debouncer debouncer(&parent, 100);

  int executed = 0;
  debouncer.schedule([&executed]() { ++executed; });
  debouncer.cancel();

  QTimer::singleShot(200, QCoreApplication::instance(), &QCoreApplication::quit);
  QCoreApplication::instance()->exec();

  EXPECT_EQ(executed, 0);
}

TEST(Debounce, RapidScheduleOnlyFiresOnce) {
  QObject parent;
  pfd::base::Debouncer debouncer(&parent, 50);

  int executed = 0;
  debouncer.schedule([&executed]() { ++executed; });
  debouncer.schedule([&executed]() { ++executed; });
  debouncer.schedule([&executed]() { ++executed; });

  QTimer::singleShot(100, QCoreApplication::instance(), &QCoreApplication::quit);
  QCoreApplication::instance()->exec();

  EXPECT_EQ(executed, 1);
}

TEST(Debounce, SetDelayAffectsSubsequentSchedule) {
  QObject parent;
  pfd::base::Debouncer debouncer(&parent, 200);
  debouncer.setDelay(50);

  int executed = 0;
  debouncer.schedule([&executed]() { ++executed; });

  QTimer::singleShot(100, QCoreApplication::instance(), &QCoreApplication::quit);
  QCoreApplication::instance()->exec();

  EXPECT_EQ(executed, 1);
}

}  // namespace
