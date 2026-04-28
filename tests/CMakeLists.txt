set(PFD_TESTS_BASE
  test_sanity.cc
  test_base_errors.cc
  test_format.cc
  test_input_sanitizer.cc
  test_io.cc
  test_process_utils.cc
  test_paths.cc
  test_threading.cc
  test_types.cc
  test_clock.cc
  test_fs_utils.cc
  test_system_info.cc
  test_time_stamps.cc
  test_time_format.cc
  test_throttle.cc
  test_debounce.cc
  test_ring_buffer.cc
)

set(PFD_TESTS_CORE
  test_log_store.cc
  test_logger.cc
  test_task_state_store.cc
  test_task_pipeline.cc
  test_task_pipeline_service.cc
  test_rss_parser.cc
  test_rss_repository.cc
  test_rss_service.cc
  test_rss_rule_engine.cc
  test_rss_series_tracker.cc
  test_rss_auto_download.cc
  test_rss_dedup.cc
  test_rss_opml.cc
  test_save_path_policy.cc
  test_task_add_builder.cc
  test_builtin_http_tracker.cc
)

set(PFD_TESTS_LT
  test_lt_error_conversions.cc
  test_lt_task_event_mapper.cc
  test_lt_task_event_dispatcher.cc
  test_lt_task_alert_adapter.cc
  test_ip_filter_loader.cc
  test_session_worker.cc
)

set(PFD_TESTS_APP
  test_task_query_mapper.cc
  test_task_control_command_use_case.cc
  test_task_meta_command_use_case.cc
  test_exit_coordinator.cc
  test_rss_download_pipeline.cc
  test_rss_download_orchestrator.cc
)

set(PFD_TESTS_UI
  test_ui_main_window.cc
)

add_executable(torrentlink_tests
  test_main.cc
  ${PFD_TESTS_BASE}
  ${PFD_TESTS_CORE}
  ${PFD_TESTS_LT}
  ${PFD_TESTS_APP}
)

target_link_libraries(torrentlink_tests
  PRIVATE
    GTest::gtest
    Qt6::Core
    torrentlink_base
    torrentlink_core
    torrentlink_lt
    torrentlink_app
)

pfd_set_target_warnings(torrentlink_tests)

add_executable(torrentlink_ui_tests
  test_main_ui.cc
  ${PFD_TESTS_UI}
)

target_link_libraries(torrentlink_ui_tests
  PRIVATE
    GTest::gtest
    Qt6::Core
    Qt6::Widgets
    Qt6::Test
    torrentlink_base
    torrentlink_core
    torrentlink_lt
    torrentlink_app
    torrentlink_ui
)

pfd_set_target_warnings(torrentlink_ui_tests)

include(GoogleTest)
gtest_discover_tests(torrentlink_tests)
gtest_discover_tests(torrentlink_ui_tests)
