#ifndef PFD_LT_TASK_EVENT_DISPATCHER_H
#define PFD_LT_TASK_EVENT_DISPATCHER_H

#include <vector>

#include "core/task_pipeline.h"
#include "lt/task_event_mapper.h"

namespace pfd::lt {

/// @brief lt 侧统一分发入口：alert view -> task event -> core pipeline
void dispatchAlertViewToPipeline(const LtAlertView& alert, pfd::core::TaskPipeline* pipeline);
void dispatchAlertViewsToPipeline(const std::vector<LtAlertView>& alerts,
                                  pfd::core::TaskPipeline* pipeline);

}  // namespace pfd::lt

#endif  // PFD_LT_TASK_EVENT_DISPATCHER_H
