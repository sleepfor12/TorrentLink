add_library(p2p_core STATIC
  app_settings.h
  app_settings.cc
  config_service.h
  config_service.cc
  rss/rss_types.h
  rss/rss_types.cc
  rss/rss_repository.h
  rss/rss_repository.cc
  rss/rss_fetcher.h
  rss/rss_fetcher.cc
  rss/rss_parser.h
  rss/rss_parser.cc
  rss/rss_dedup.h
  rss/rss_dedup.cc
  rss/rss_opml.h
  rss/rss_opml.cc
  rss/rss_rule_engine.h
  rss/rss_rule_engine.cc
  rss/rss_series_tracker.h
  rss/rss_series_tracker.cc
  rss/rss_service.h
  rss/rss_service.cc
  save_path_policy.h
  save_path_policy.cc
  task_add_builder.h
  task_add_builder.cc
  external_player.h
  external_player.cc
  logger.h
  logger.cc
  log_store.h
  log_store.cc
  task_event.h
  task_snapshot.h
  task_snapshot.cc
  task_state_store.h
  task_state_store.cc
  task_pipeline.h
  task_pipeline.cc
  task_pipeline_service.h
  task_pipeline_service.cc
)

target_include_directories(p2p_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(p2p_core
  PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Network
    p2p_base
)

target_compile_features(p2p_core PUBLIC cxx_std_17)
pfd_set_target_warnings(p2p_core)

