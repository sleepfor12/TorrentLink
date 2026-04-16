#include "lt/task_event_mapper.h"

namespace pfd::lt {

std::optional<pfd::core::TaskEvent> mapAlertToTaskEvent(const LtAlertView& alert) {
  if (alert.type == LtAlertView::Type::kIgnored || alert.taskId.isNull()) {
    return std::nullopt;
  }

  pfd::core::TaskEvent ev;
  ev.taskId = alert.taskId;
  ev.eventMs = alert.eventMs;

  switch (alert.type) {
    case LtAlertView::Type::kTaskAdded:
      ev.type = pfd::core::TaskEventType::kUpsertMeta;
      ev.name = alert.name;
      ev.savePath = alert.savePath;
      ev.infoHashV1 = alert.infoHashV1;
      ev.infoHashV2 = alert.infoHashV2;
      ev.comment = alert.comment;
      ev.totalBytes = alert.totalBytes;
      ev.selectedBytes = alert.selectedBytes;
      ev.pieces = alert.pieces;
      ev.pieceLength = alert.pieceLength;
      ev.createdBy = alert.createdBy;
      ev.createdOnMs = alert.createdOnMs;
      ev.isPrivate = alert.isPrivate;
      return ev;

    case LtAlertView::Type::kTaskStateChanged:
      ev.type = pfd::core::TaskEventType::kStatusChanged;
      ev.status = alert.status;
      ev.errorMessage = alert.message;
      return ev;

    case LtAlertView::Type::kTaskProgress:
      ev.type = pfd::core::TaskEventType::kProgressUpdated;
      ev.name = alert.name;
      ev.status = alert.status;
      ev.totalBytes = alert.totalBytes;
      ev.selectedBytes = alert.selectedBytes;
      ev.downloadedBytes = alert.downloadedBytes;
      ev.uploadedBytes = alert.uploadedBytes;
      ev.downloadRate = alert.downloadRate;
      ev.uploadRate = alert.uploadRate;
      ev.seeders = alert.seeders;
      ev.users = alert.users;
      ev.availability = alert.availability;
      ev.infoHashV1 = alert.infoHashV1;
      ev.infoHashV2 = alert.infoHashV2;
      ev.activeTimeSec = alert.activeTimeSec;
      ev.dlLimitBps = alert.dlLimitBps;
      ev.ulLimitBps = alert.ulLimitBps;
      ev.wastedBytes = alert.wastedBytes;
      ev.nextAnnounceSec = alert.nextAnnounceSec;
      ev.lastSeenCompleteMs = alert.lastSeenCompleteMs;
      ev.pieces = alert.pieces;
      ev.pieceLength = alert.pieceLength;
      ev.createdBy = alert.createdBy;
      ev.createdOnMs = alert.createdOnMs;
      ev.completedOnMs = alert.completedOnMs;
      ev.isPrivate = alert.isPrivate;
      return ev;

    case LtAlertView::Type::kTaskRate:
      ev.type = pfd::core::TaskEventType::kRateUpdated;
      ev.downloadRate = alert.downloadRate;
      ev.uploadRate = alert.uploadRate;
      return ev;

    case LtAlertView::Type::kTaskError:
      ev.type = pfd::core::TaskEventType::kErrorOccurred;
      ev.errorMessage = alert.message;
      return ev;

    case LtAlertView::Type::kDuplicateRejected:
      ev.type = pfd::core::TaskEventType::kDuplicateRejected;
      ev.errorMessage = alert.message;
      return ev;

    case LtAlertView::Type::kTaskRemoved:
      ev.type = pfd::core::TaskEventType::kRemoved;
      return ev;

    case LtAlertView::Type::kIgnored:
      return std::nullopt;
  }

  return std::nullopt;
}

}  // namespace pfd::lt
