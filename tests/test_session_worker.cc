#include <QtCore/QUuid>

#include <atomic>
#include <chrono>
#include <gtest/gtest.h>
#include <mutex>
#include <thread>
#include <vector>

#include "lt/session_worker.h"
#include "lt/task_event_mapper.h"

namespace {

TEST(SessionWorker, ConstructAndDestruct) {
  pfd::lt::SessionWorker worker;
}

TEST(SessionWorker, ControlCommandsWithUnknownTaskDoNotCrash) {
  pfd::lt::SessionWorker worker;
  const auto unknownId = QUuid::createUuid();

  worker.pauseTask(unknownId);
  worker.resumeTask(unknownId);
  worker.removeTask(unknownId, false);
  worker.setTaskFilePriority(unknownId, {0, 1, 2},
                             pfd::lt::SessionWorker::FilePriorityLevel::kHigh);
  worker.renameTaskFileOrFolder(unknownId, QStringLiteral("folder/file.bin"),
                                QStringLiteral("renamed.bin"));
}

TEST(SessionWorker, InvalidAddInputsDoNotEmitAlerts) {
  pfd::lt::SessionWorker worker;
  std::atomic<int> callbacks{0};
  worker.setAlertsCallback([&callbacks](std::vector<pfd::lt::LtAlertView> views) {
    if (!views.empty()) {
      ++callbacks;
    }
  });

  worker.addMagnet(QStringLiteral(""), QStringLiteral("/tmp"));
  worker.addMagnet(QStringLiteral("not-a-magnet"), QStringLiteral("/tmp"));
  worker.addTorrentFile(QStringLiteral("/tmp/not-exists.torrent"), QStringLiteral("/tmp"));

  std::this_thread::sleep_for(std::chrono::milliseconds(400));
  EXPECT_EQ(callbacks.load(), 0);
}

TEST(SessionWorker, QueryTaskFilesUnknownTaskReturnsEmpty) {
  pfd::lt::SessionWorker worker;
  const auto unknownId = QUuid::createUuid();
  const auto files = worker.queryTaskFiles(unknownId);
  EXPECT_TRUE(files.empty());
}

TEST(SessionWorker, DuplicateMagnetSecondAddEmitsErrorView) {
  pfd::lt::SessionWorker worker;
  std::mutex mu;
  std::vector<pfd::lt::LtAlertView> all;
  worker.setAlertsCallback([&](std::vector<pfd::lt::LtAlertView> views) {
    std::lock_guard<std::mutex> lk(mu);
    all.insert(all.end(), views.begin(), views.end());
  });

  const QString magnet =
      QStringLiteral("magnet:?xt=urn:btih:08ada5a7a618133027292e316d4054bfb324ad96");
  worker.addMagnet(magnet, QStringLiteral("/tmp"));

  bool sawAdded = false;
  for (int i = 0; i < 80 && !sawAdded; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::lock_guard<std::mutex> lk(mu);
    for (const auto& x : all) {
      if (x.type == pfd::lt::LtAlertView::Type::kTaskAdded) {
        sawAdded = true;
        break;
      }
    }
  }
  ASSERT_TRUE(sawAdded) << "first add should produce kTaskAdded";

  worker.addMagnet(magnet, QStringLiteral("/tmp"));
  bool sawDup = false;
  for (int i = 0; i < 40 && !sawDup; ++i) {
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
    std::lock_guard<std::mutex> lk(mu);
    for (const auto& x : all) {
      if (x.type == pfd::lt::LtAlertView::Type::kDuplicateRejected &&
          x.message.contains(QStringLiteral("info-hash"))) {
        sawDup = true;
        break;
      }
    }
  }
  EXPECT_TRUE(sawDup);
}

}  // namespace
