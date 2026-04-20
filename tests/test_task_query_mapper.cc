#include <gtest/gtest.h>

#include "app/task_query_mapper.h"

namespace {

TEST(TaskQueryMapper, MapsFileEntriesAndPriority) {
  pfd::lt::SessionWorker::FileEntrySnapshot src;
  src.fileIndex = 3;
  src.logicalPath = QStringLiteral("dir/file.bin");
  src.sizeBytes = 100;
  src.downloadedBytes = 50;
  src.progress01 = 0.5;
  src.availability = 1.25;
  src.priority = pfd::lt::SessionWorker::FilePriorityLevel::kHigh;

  const auto out = pfd::app::mapTaskFiles({src});
  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0].fileIndex, 3);
  EXPECT_EQ(out[0].logicalPath, QStringLiteral("dir/file.bin"));
  EXPECT_EQ(out[0].priority, pfd::core::TaskFilePriorityLevel::kHigh);
  EXPECT_EQ(pfd::app::mapTaskFilePriorityLevel(pfd::core::TaskFilePriorityLevel::kFileOrder),
            pfd::lt::SessionWorker::FilePriorityLevel::kFileOrder);
}

TEST(TaskQueryMapper, MapsTrackerPeerAndWebSeedDtos) {
  pfd::lt::SessionWorker::TrackerRowSnapshot row;
  row.url = QStringLiteral("udp://tracker");
  row.status = pfd::lt::SessionWorker::TrackerStatus::kWorking;
  pfd::lt::SessionWorker::TrackerEndpointSnapshot ep;
  ep.ip = QStringLiteral("1.1.1.1");
  ep.port = 80;
  ep.status = pfd::lt::SessionWorker::TrackerStatus::kUpdating;
  row.endpoints.push_back(ep);
  pfd::lt::SessionWorker::TaskTrackerSnapshot trackers;
  trackers.trackers.push_back(row);

  const auto trackerOut = pfd::app::mapTaskTrackers(trackers);
  ASSERT_EQ(trackerOut.trackers.size(), 1u);
  EXPECT_EQ(trackerOut.trackers[0].status, pfd::core::TaskTrackerStatusDto::kWorking);
  ASSERT_EQ(trackerOut.trackers[0].endpoints.size(), 1u);
  EXPECT_EQ(trackerOut.trackers[0].endpoints[0].status, pfd::core::TaskTrackerStatusDto::kUpdating);

  pfd::lt::SessionWorker::PeerSnapshot peer;
  peer.client = QStringLiteral("uTorrent");
  const auto peers = pfd::app::mapTaskPeers({peer});
  ASSERT_EQ(peers.size(), 1u);
  EXPECT_EQ(peers[0].client, QStringLiteral("uTorrent"));

  pfd::lt::SessionWorker::WebSeedSnapshot seed;
  seed.url = QStringLiteral("https://seed");
  seed.type = QStringLiteral("BEP19");
  const auto seeds = pfd::app::mapTaskWebSeeds({seed});
  ASSERT_EQ(seeds.size(), 1u);
  EXPECT_EQ(seeds[0].type, QStringLiteral("BEP19"));
}

}  // namespace
