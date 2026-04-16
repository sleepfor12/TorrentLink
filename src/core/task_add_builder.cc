#include "core/task_add_builder.h"

#include "core/save_path_policy.h"

namespace pfd::core {

TaskAddResult buildTaskAdd(const TaskAddInput& in, const SavePathPolicy& policy) {
  TaskAddResult r;
  const QString resolved = policy.resolve(in.savePath);
  r.finalSavePath = policy.applyLayout(resolved, in.name, in.layout);

  r.opts.start_torrent = in.startTorrent;
  r.opts.stop_when_ready = (in.stopCondition == 1);
  r.opts.sequential_download = in.sequentialDownload;
  r.opts.skip_hash_check = in.skipHashCheck;
  r.opts.add_to_top_queue = in.addToTopQueue;
  r.opts.category = in.category;
  r.opts.tags_csv = in.tagsCsv;
  r.opts.file_priorities.reserve(in.fileWanted.size());
  for (int w : in.fileWanted) {
    r.opts.file_priorities.push_back(static_cast<std::uint8_t>(w ? 4 : 0));
  }
  return r;
}

}  // namespace pfd::core
