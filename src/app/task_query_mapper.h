#ifndef PFD_APP_TASK_QUERY_MAPPER_H
#define PFD_APP_TASK_QUERY_MAPPER_H

#include <vector>

#include "core/task_query_dto.h"
#include "lt/session_worker.h"

namespace pfd::app {

std::vector<pfd::core::TaskFileEntryDto>
mapTaskFiles(const std::vector<pfd::lt::SessionWorker::FileEntrySnapshot>& input);

std::vector<pfd::lt::SessionWorker::FilePriorityLevel> mapTaskFilePriorityLevels();

pfd::core::TaskTrackerSnapshotDto
mapTaskTrackers(const pfd::lt::SessionWorker::TaskTrackerSnapshot& input);

std::vector<pfd::core::TaskPeerDto>
mapTaskPeers(const std::vector<pfd::lt::SessionWorker::PeerSnapshot>& input);

std::vector<pfd::core::TaskWebSeedDto>
mapTaskWebSeeds(const std::vector<pfd::lt::SessionWorker::WebSeedSnapshot>& input);

pfd::lt::SessionWorker::FilePriorityLevel
mapTaskFilePriorityLevel(pfd::core::TaskFilePriorityLevel p);

}  // namespace pfd::app

#endif  // PFD_APP_TASK_QUERY_MAPPER_H
