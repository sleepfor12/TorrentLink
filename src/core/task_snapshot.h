#ifndef PFD_CORE_TASK_SNAPSHOT_H
#define PFD_CORE_TASK_SNAPSHOT_H

#include <QtCore/QString>
#include <QtCore/QtGlobal>

#include "base/time_stamps.h"
#include "base/types.h"

namespace pfd::core {

/// @brief Core 对 UI/调用方暴露的统一任务快照契约
struct TaskSnapshot {
  pfd::base::TaskId taskId;
  pfd::base::TaskStatus status{pfd::base::TaskStatus::kUnknown};

  QString name;
  QString savePath;
  QString errorMessage;
  QString category;
  QString tags;
  QString infoHashV1;
  QString infoHashV2;
  QString comment;

  qint64 totalBytes{0};
  qint64 selectedBytes{0};
  qint64 downloadedBytes{0};
  qint64 uploadedBytes{0};
  qint64 downloadRate{0};  // bytes/s
  qint64 uploadRate{0};    // bytes/s
  qint64 seeders{0};
  qint64 users{0};
  double availability{0.0};
  qint64 addedAtMs{pfd::base::invalidMs()};

  // Extended fields
  qint64 activeTimeSec{0};
  qint64 dlLimitBps{0};
  qint64 ulLimitBps{0};
  qint64 wastedBytes{0};
  qint64 nextAnnounceSec{-1};
  qint64 lastSeenCompleteMs{pfd::base::invalidMs()};
  int pieces{0};
  int pieceLength{0};
  QString createdBy;
  qint64 createdOnMs{pfd::base::invalidMs()};
  qint64 completedOnMs{pfd::base::invalidMs()};
  int isPrivate{-1};  // -1=unknown, 0=no, 1=yes

  qint64 lastUpdatedMs{pfd::base::invalidMs()};

  [[nodiscard]] double progress01() const;
};

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_SNAPSHOT_H
