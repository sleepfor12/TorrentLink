#ifndef PFD_LT_SESSION_OPS_H
#define PFD_LT_SESSION_OPS_H

#include <QtCore/QString>

#include <libtorrent/torrent_handle.hpp>

#include <future>
#include <map>
#include <memory>
#include <optional>

#include "lt/add_torrent_torrent_info_ptr.h"
#include "lt/session_cmds.h"
#include "lt/session_worker.h"

namespace libtorrent {
struct session;
}  // namespace libtorrent

namespace pfd::lt::session_ops {

struct Context {
  std::map<QString, libtorrent::torrent_handle>& handlesByTaskId;
  int& defaultPerTorrentConnectionsLimit;
  std::map<QString, int>& perTaskConnectionsLimit;
  std::map<QString, SessionWorker::AddTorrentOptions>& pendingAddOpts;
  std::map<QString, pfd::lt::AddTorrentTorrentInfoConstPtr>& preparedTorrentInfo;
  std::map<QString, std::shared_ptr<std::promise<std::optional<SessionWorker::MagnetMetadata>>>>&
      pendingMagnetMeta;
  int& pendingResumeDataSaves;
  int& completedResumeDataSaves;
  QString& resumeDataDir;
  std::shared_ptr<std::promise<int>>& resumeDataDone;
  /// 非 libtorrent alert 产生的视图（例如重复 info-hash 拒绝），由 SessionWorker 合并进回调批次。
  std::vector<LtAlertView>* syntheticAlertViews{nullptr};
  /// 若非空，指向 worker 内待完成的会话统计查询；由 SessionWorker 在 post_session_stats 后兑现。
  std::shared_ptr<std::promise<SessionWorker::SessionStats>>* pendingSessionStats{nullptr};
};

// Returns true if cmd is handled (and run() should continue).
// Returns false when cmd should be handled by caller (e.g. Shutdown).
bool handleCmd(libtorrent::session& ses, Context& ctx, const session_cmds::Cmd& cmd);

}  // namespace pfd::lt::session_ops

#endif  // PFD_LT_SESSION_OPS_H
