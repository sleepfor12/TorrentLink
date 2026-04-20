#ifndef PFD_CORE_TASK_QUERY_DTO_H
#define PFD_CORE_TASK_QUERY_DTO_H

#include <QtCore/QString>

#include <cstdint>
#include <vector>

namespace pfd::core {

enum class TaskFilePriorityLevel : std::uint8_t {
  kNotDownloaded = 0,
  kDoNotDownload,
  kNormal,
  kHigh,
  kHighest,
  kFileOrder,
};

struct TaskFileEntryDto {
  int fileIndex{-1};
  QString logicalPath;
  qint64 sizeBytes{0};
  qint64 downloadedBytes{0};
  double progress01{0.0};
  double availability{-1.0};
  TaskFilePriorityLevel priority{TaskFilePriorityLevel::kNormal};
};

enum class TaskTrackerStatusDto : std::uint8_t {
  kCannotConnect = 0,
  kNotWorking,
  kWorking,
  kUpdating,
};

struct TaskTrackerEndpointDto {
  QString ip;
  int port{0};
  QString btProtocol;
  TaskTrackerStatusDto status{TaskTrackerStatusDto::kNotWorking};
  int users{-1};
  int seeds{-1};
  int downloads{-1};
  int nextAnnounceSec{-1};
  int minAnnounceSec{-1};
  QString message;
};

struct TaskTrackerRowDto {
  QString url;
  int tier{0};
  QString btProtocol;
  TaskTrackerStatusDto status{TaskTrackerStatusDto::kNotWorking};
  int users{-1};
  int seeds{-1};
  int downloads{-1};
  int nextAnnounceSec{-1};
  int minAnnounceSec{-1};
  QString message;
  bool readOnly{false};
  std::vector<TaskTrackerEndpointDto> endpoints;
};

struct TaskTrackerSnapshotDto {
  std::vector<TaskTrackerRowDto> fixedRows;
  std::vector<TaskTrackerRowDto> trackers;
};

struct TaskPeerDto {
  QString ip;
  int port{0};
  QString client;
  double progress{0.0};
  qint64 downloadRate{0};
  qint64 uploadRate{0};
  qint64 totalDownloaded{0};
  qint64 totalUploaded{0};
  QString flags;
  QString connection;
};

struct TaskWebSeedDto {
  QString url;
  QString type;
};

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_QUERY_DTO_H
