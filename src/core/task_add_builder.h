#ifndef PFD_CORE_TASK_ADD_BUILDER_H
#define PFD_CORE_TASK_ADD_BUILDER_H

#include <QtCore/QString>

#include <cstdint>
#include <vector>

namespace pfd::core {

enum class ContentLayout;

struct TaskAddInput {
  QString name;
  QString savePath;
  ContentLayout layout;
  bool startTorrent{true};
  int stopCondition{0};
  bool sequentialDownload{false};
  bool skipHashCheck{false};
  bool addToTopQueue{false};
  QString category;
  QString tagsCsv;
  std::vector<int> fileWanted;
};

struct TaskAddOutput {
  std::vector<std::uint8_t> file_priorities;
  bool start_torrent{true};
  bool stop_when_ready{false};
  bool sequential_download{false};
  bool skip_hash_check{false};
  bool add_to_top_queue{false};
  QString category;
  QString tags_csv;
};

struct TaskAddResult {
  TaskAddOutput opts;
  QString finalSavePath;
};

class SavePathPolicy;

TaskAddResult buildTaskAdd(const TaskAddInput& in, const SavePathPolicy& policy);

}  // namespace pfd::core

#endif  // PFD_CORE_TASK_ADD_BUILDER_H
