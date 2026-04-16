#ifndef PFD_LT_TASK_EVENT_MAPPER_H
#define PFD_LT_TASK_EVENT_MAPPER_H

#include <QtCore/QString>

#include <optional>

#include "base/types.h"
#include "core/task_event.h"

namespace pfd::lt {

/// @brief 面向 lt alert 的稳定输入视图（避免 core 直接依赖 libtorrent 类型）
struct LtAlertView {
  enum class Type : std::uint8_t {
    kTaskAdded = 0,
    kTaskStateChanged,
    kTaskProgress,
    kTaskRate,
    kTaskError,
    kDuplicateRejected,
    kTaskRemoved,
    kIgnored,
  };

  Type type{Type::kIgnored};
  pfd::base::TaskId taskId;

  pfd::base::TaskStatus status{pfd::base::TaskStatus::kUnknown};
  QString name;
  QString savePath;
  QString message;
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
  QString magnet;
  QString infoHashV1;
  QString infoHashV2;
  QString comment;
  qint64 eventMs{-1};

  // Extended fields (Phase 1)
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
  int isPrivate{-1};  // -1=unknown, 0=no, 1=yes
};

/// @brief 将 lt 侧 alert 视图映射为 core::TaskEvent；无效/忽略事件返回 nullopt
[[nodiscard]] std::optional<pfd::core::TaskEvent> mapAlertToTaskEvent(const LtAlertView& alert);

}  // namespace pfd::lt

#endif  // PFD_LT_TASK_EVENT_MAPPER_H
