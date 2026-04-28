add_library(torrentlink_app STATIC
  task_batch_use_case.h
  task_batch_use_case.cc
  rss_download_pipeline.h
  rss_download_pipeline.cc
  rss_download_orchestrator.h
  rss_download_orchestrator.cc
  exit_coordinator.h
  exit_coordinator.cc
  event_ingest_orchestrator.h
  event_ingest_orchestrator.cc
  refresh_scheduler.h
  refresh_scheduler.cc
  task_persistence_coordinator.h
  task_persistence_coordinator.cc
  task_query_mapper.h
  task_query_mapper.cc
  task_detail_query_use_case.h
  task_detail_query_use_case.cc
  task_control_command_use_case.h
  task_control_command_use_case.cc
  task_meta_command_use_case.h
  task_meta_command_use_case.cc
)

target_include_directories(torrentlink_app
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(torrentlink_app
  PUBLIC
    Qt6::Core
    torrentlink_base
    torrentlink_core
    torrentlink_lt
)

target_compile_features(torrentlink_app PUBLIC cxx_std_17)
pfd_set_target_warnings(torrentlink_app)

