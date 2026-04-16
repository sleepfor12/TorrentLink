#include "core/task_state_store.h"

#include "base/time_stamps.h"

namespace pfd::core {

namespace {

qint64 pickEventTimeOrNow(qint64 eventMs) {
  return pfd::base::isValidMs(eventMs) ? eventMs : pfd::base::currentMs();
}

}  // namespace

void TaskStateStore::applyEvent(const TaskEvent& event) {
  std::lock_guard<std::mutex> guard(mu_);

  if (event.type == TaskEventType::kDuplicateRejected) {
    return;
  }

  if (event.type == TaskEventType::kRemoved) {
    tasks_.erase(event.taskId);
    return;
  }

  auto [it, inserted] = tasks_.try_emplace(event.taskId);
  TaskSnapshot& s = it->second;
  if (inserted) {
    s.taskId = event.taskId;
    s.addedAtMs = pickEventTimeOrNow(event.eventMs);
  }

  switch (event.type) {
    case TaskEventType::kUpsertMeta:
      if (!event.name.isEmpty()) {
        s.name = event.name;
      }
      if (!event.savePath.isEmpty()) {
        s.savePath = event.savePath;
      }
      if (!event.category.isEmpty()) {
        s.category = event.category;
      }
      if (!event.tags.isEmpty()) {
        s.tags = event.tags;
      }
      if (!event.infoHashV1.isEmpty()) {
        s.infoHashV1 = event.infoHashV1;
      }
      if (!event.infoHashV2.isEmpty()) {
        s.infoHashV2 = event.infoHashV2;
      }
      if (!event.comment.isEmpty()) {
        s.comment = event.comment;
      }
      if (event.totalBytes >= 0) {
        s.totalBytes = event.totalBytes;
      }
      if (event.selectedBytes >= 0) {
        s.selectedBytes = event.selectedBytes;
      }
      if (event.pieces >= 0) {
        s.pieces = event.pieces;
      }
      if (event.pieceLength >= 0) {
        s.pieceLength = event.pieceLength;
      }
      if (!event.createdBy.isEmpty()) {
        s.createdBy = event.createdBy;
      }
      if (event.createdOnMs >= 0) {
        s.createdOnMs = event.createdOnMs;
      }
      if (event.isPrivate >= 0) {
        s.isPrivate = event.isPrivate;
      }
      break;

    case TaskEventType::kStatusChanged:
      s.status = event.status;
      if (!event.errorMessage.isEmpty()) {
        s.errorMessage = event.errorMessage;
      }
      break;

    case TaskEventType::kProgressUpdated:
      if (!event.name.isEmpty()) {
        s.name = event.name;
      }
      if (event.status != pfd::base::TaskStatus::kUnknown) {
        s.status = event.status;
      }
      if (event.totalBytes >= 0) {
        s.totalBytes = event.totalBytes;
      }
      if (event.selectedBytes >= 0) {
        s.selectedBytes = event.selectedBytes;
      }
      if (event.downloadedBytes >= 0) {
        s.downloadedBytes = event.downloadedBytes;
      }
      if (event.uploadedBytes >= 0) {
        s.uploadedBytes = event.uploadedBytes;
      }
      if (event.downloadRate >= 0) {
        s.downloadRate = event.downloadRate;
      }
      if (event.uploadRate >= 0) {
        s.uploadRate = event.uploadRate;
      }
      if (event.seeders >= 0) {
        s.seeders = event.seeders;
      }
      if (event.users >= 0) {
        s.users = event.users;
      }
      if (event.availability >= 0.0) {
        s.availability = event.availability;
      }
      if (!event.category.isEmpty()) {
        s.category = event.category;
      }
      if (!event.tags.isEmpty()) {
        s.tags = event.tags;
      }
      if (!event.infoHashV1.isEmpty()) {
        s.infoHashV1 = event.infoHashV1;
      }
      if (!event.infoHashV2.isEmpty()) {
        s.infoHashV2 = event.infoHashV2;
      }
      if (event.activeTimeSec >= 0) {
        s.activeTimeSec = event.activeTimeSec;
      }
      if (event.dlLimitBps >= 0) {
        s.dlLimitBps = event.dlLimitBps;
      }
      if (event.ulLimitBps >= 0) {
        s.ulLimitBps = event.ulLimitBps;
      }
      if (event.wastedBytes >= 0) {
        s.wastedBytes = event.wastedBytes;
      }
      if (event.nextAnnounceSec >= 0) {
        s.nextAnnounceSec = event.nextAnnounceSec;
      }
      if (event.lastSeenCompleteMs >= 0) {
        s.lastSeenCompleteMs = event.lastSeenCompleteMs;
      }
      if (event.pieces >= 0) {
        s.pieces = event.pieces;
      }
      if (event.pieceLength >= 0) {
        s.pieceLength = event.pieceLength;
      }
      if (!event.createdBy.isEmpty()) {
        s.createdBy = event.createdBy;
      }
      if (event.createdOnMs >= 0) {
        s.createdOnMs = event.createdOnMs;
      }
      if (event.completedOnMs >= 0) {
        s.completedOnMs = event.completedOnMs;
      }
      if (event.isPrivate >= 0) {
        s.isPrivate = event.isPrivate;
      }
      break;

    case TaskEventType::kRateUpdated:
      if (event.downloadRate >= 0) {
        s.downloadRate = event.downloadRate;
      }
      if (event.uploadRate >= 0) {
        s.uploadRate = event.uploadRate;
      }
      break;

    case TaskEventType::kErrorOccurred:
      s.status = pfd::base::TaskStatus::kError;
      s.errorMessage = event.errorMessage;
      break;

    case TaskEventType::kDuplicateRejected:
      break;

    case TaskEventType::kRemoved:
      // handled above
      break;
  }

  s.lastUpdatedMs = pickEventTimeOrNow(event.eventMs);
}

std::optional<TaskSnapshot> TaskStateStore::snapshot(const pfd::base::TaskId& taskId) const {
  std::lock_guard<std::mutex> guard(mu_);
  auto it = tasks_.find(taskId);
  if (it == tasks_.end()) {
    return std::nullopt;
  }
  return it->second;
}

std::vector<TaskSnapshot> TaskStateStore::snapshots() const {
  std::lock_guard<std::mutex> guard(mu_);
  std::vector<TaskSnapshot> out;
  out.reserve(tasks_.size());
  for (const auto& [id, snapshot] : tasks_) {
    (void)id;
    out.push_back(snapshot);
  }
  return out;
}

}  // namespace pfd::core
