#include "app/task_query_mapper.h"

namespace pfd::app {

namespace {

pfd::core::TaskFilePriorityLevel toDto(pfd::lt::SessionWorker::FilePriorityLevel p) {
  return static_cast<pfd::core::TaskFilePriorityLevel>(p);
}

pfd::core::TaskTrackerStatusDto toDto(pfd::lt::SessionWorker::TrackerStatus s) {
  return static_cast<pfd::core::TaskTrackerStatusDto>(s);
}

}  // namespace

std::vector<pfd::core::TaskFileEntryDto>
mapTaskFiles(const std::vector<pfd::lt::SessionWorker::FileEntrySnapshot>& input) {
  std::vector<pfd::core::TaskFileEntryDto> out;
  out.reserve(input.size());
  for (const auto& src : input) {
    pfd::core::TaskFileEntryDto dst;
    dst.fileIndex = src.fileIndex;
    dst.logicalPath = src.logicalPath;
    dst.sizeBytes = src.sizeBytes;
    dst.downloadedBytes = src.downloadedBytes;
    dst.progress01 = src.progress01;
    dst.availability = src.availability;
    dst.priority = toDto(src.priority);
    out.push_back(std::move(dst));
  }
  return out;
}

pfd::core::TaskTrackerSnapshotDto
mapTaskTrackers(const pfd::lt::SessionWorker::TaskTrackerSnapshot& input) {
  auto mapRows = [](const std::vector<pfd::lt::SessionWorker::TrackerRowSnapshot>& rows) {
    std::vector<pfd::core::TaskTrackerRowDto> out;
    out.reserve(rows.size());
    for (const auto& srcRow : rows) {
      pfd::core::TaskTrackerRowDto dstRow;
      dstRow.url = srcRow.url;
      dstRow.tier = srcRow.tier;
      dstRow.btProtocol = srcRow.btProtocol;
      dstRow.status = toDto(srcRow.status);
      dstRow.users = srcRow.users;
      dstRow.seeds = srcRow.seeds;
      dstRow.downloads = srcRow.downloads;
      dstRow.nextAnnounceSec = srcRow.nextAnnounceSec;
      dstRow.minAnnounceSec = srcRow.minAnnounceSec;
      dstRow.message = srcRow.message;
      dstRow.readOnly = srcRow.readOnly;
      for (const auto& ep : srcRow.endpoints) {
        pfd::core::TaskTrackerEndpointDto dstEp;
        dstEp.ip = ep.ip;
        dstEp.port = ep.port;
        dstEp.btProtocol = ep.btProtocol;
        dstEp.status = toDto(ep.status);
        dstEp.users = ep.users;
        dstEp.seeds = ep.seeds;
        dstEp.downloads = ep.downloads;
        dstEp.nextAnnounceSec = ep.nextAnnounceSec;
        dstEp.minAnnounceSec = ep.minAnnounceSec;
        dstEp.message = ep.message;
        dstRow.endpoints.push_back(std::move(dstEp));
      }
      out.push_back(std::move(dstRow));
    }
    return out;
  };

  pfd::core::TaskTrackerSnapshotDto out;
  out.fixedRows = mapRows(input.fixedRows);
  out.trackers = mapRows(input.trackers);
  return out;
}

std::vector<pfd::core::TaskPeerDto>
mapTaskPeers(const std::vector<pfd::lt::SessionWorker::PeerSnapshot>& input) {
  std::vector<pfd::core::TaskPeerDto> out;
  out.reserve(input.size());
  for (const auto& src : input) {
    pfd::core::TaskPeerDto dst;
    dst.ip = src.ip;
    dst.port = src.port;
    dst.client = src.client;
    dst.progress = src.progress;
    dst.downloadRate = src.downloadRate;
    dst.uploadRate = src.uploadRate;
    dst.totalDownloaded = src.totalDownloaded;
    dst.totalUploaded = src.totalUploaded;
    dst.flags = src.flags;
    dst.connection = src.connection;
    out.push_back(std::move(dst));
  }
  return out;
}

std::vector<pfd::core::TaskWebSeedDto>
mapTaskWebSeeds(const std::vector<pfd::lt::SessionWorker::WebSeedSnapshot>& input) {
  std::vector<pfd::core::TaskWebSeedDto> out;
  out.reserve(input.size());
  for (const auto& src : input) {
    pfd::core::TaskWebSeedDto dst;
    dst.url = src.url;
    dst.type = src.type;
    out.push_back(std::move(dst));
  }
  return out;
}

pfd::lt::SessionWorker::FilePriorityLevel
mapTaskFilePriorityLevel(pfd::core::TaskFilePriorityLevel p) {
  return static_cast<pfd::lt::SessionWorker::FilePriorityLevel>(p);
}

}  // namespace pfd::app
