#include <libtorrent/session.hpp>

#include <utility>

#include "lt/session_ops.h"

namespace pfd::lt::session_ops {
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::AddMagnetCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::PrepareMagnetMetadataCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::CancelPreparedMagnetCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::FinalizePreparedMagnetCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::AddTorrentFileCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::AddFromResumeDataCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::PauseCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::ResumeCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::RemoveCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::MoveStorageCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::EditTrackersCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::ForceRecheckCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::ForceReannounceCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::SetSequentialDownloadCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::SetAutoManagedCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::ForceStartCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::SetFirstLastPiecePriorityCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::QueuePositionUpCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueuePositionDownCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueuePositionTopCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueuePositionBottomCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::ApplyRuntimeSettingsCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::SetDefaultPerTorrentConnectionsLimitCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::SetTaskConnectionsLimitCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueryTaskCopyPayloadCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QuerySessionStatsCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::QueryTaskFilesCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueryTaskTrackersCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueryTaskPeersCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::QueryTaskWebSeedsCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::SetTaskFilePriorityCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::RenameTaskFileOrFolderCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::AddTaskTrackerCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::EditTaskTrackerCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::RemoveTaskTrackerCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::ForceReannounceTrackerCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::ForceReannounceAllTrackersCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx,
               const session_cmds::SaveAllResumeDataCmd& cmd);
bool handleOne(libtorrent::session& ses, Context& ctx, const session_cmds::ShutdownCmd& cmd);

bool handleCmd(libtorrent::session& ses, Context& ctx, const session_cmds::Cmd& cmd) {
  return std::visit([&](const auto& c) -> bool { return handleOne(ses, ctx, c); }, cmd);
}

}  // namespace pfd::lt::session_ops
