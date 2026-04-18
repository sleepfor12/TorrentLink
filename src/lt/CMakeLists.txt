add_library(torrentlink_lt STATIC
  add_torrent_torrent_info_ptr.h
  error_conversions.h
  error_conversions.cc
  task_event_mapper.h
  task_event_mapper.cc
  task_event_dispatcher.h
  task_event_dispatcher.cc
  task_alert_adapter.h
  task_alert_adapter.cc
  session_cmds.h
  session_ids.h
  session_ops.h
  session_ops_dispatch.cc
  session_ops_add.cc
  session_ops_control.cc
  session_ops_settings.cc
  session_ops_query.cc
  session_worker.h
  session_worker.cc
)

target_include_directories(torrentlink_lt
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(torrentlink_lt
  PUBLIC
    Qt6::Core
    torrentlink_base
    ${PFD_LIBTORRENT_TARGET}
)

target_compile_features(torrentlink_lt PUBLIC cxx_std_17)
pfd_set_target_warnings(torrentlink_lt)

