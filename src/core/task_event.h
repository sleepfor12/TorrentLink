#ifndef PFD_CORE_TASK_EVENT_H
#define PFD_CORE_TASK_EVENT_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>

#include "base/time_stamps.h"
#include "base/types.h"

namespace pfd::core {

enum class TaskEventType : std::uint8_t {
  kUpsertMeta = 0,
  kStatusChanged,
  kProgressUpdated,
  kRateUpdated,
  kErrorOccurred,
  kDuplicateRejected,
  kRemoved,
};

/// @brief lt->core 输入事件统一契约（最小可用版本）
struct TaskEvent {
  TaskEventType type{TaskEventType::kUpsertMeta};
  pfd::base::TaskId taskId;

  // 可选 payload（按 type 使用）
  pfd::base::TaskStatus status{pfd::base::TaskStatus::kUnknown};
  QString name;
  QString savePath;
  QString errorMessage;
  QString infoHashV1;
  QString infoHashV2;
  QString comment;
  qint64 totalBytes{-1};
  qint64 selectedBytes{-1};
  qint64 downloadedBytes{-1};
  qint64 uploadedBytes{-1};
  qint64 downloadRate{-1};
  qint64 uploadRate{-1};
  qint64 seeders{-1};
  qint64 users{-1};
  double availability{-1.0};
  QString category;
  QString tags;
  qint64 eventMs{pfd::base::invalidMs()};

  // Extended fields
  qint64 activeTimeSec{-1};
  qint64 dlLimitBps{-1};
  qint64 ulLimitBps{-1};
  qint64 wastedBytes{-1};
  qint64 nextAnnounceSec{-1};
  qint64 lastSeenCompleteMs{-1};
  int pieces{-1};
  int pieceLength{-1};
  QString createdBy;
  qint64 createdOnMs{-1};
  qint64 completedOnMs{-1};
  int isPrivate{-1};
};

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_EVENT_H
