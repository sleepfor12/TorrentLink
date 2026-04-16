cmake_minimum_required(VERSION 3.22)

project(p2p_file_download_system
  VERSION 0.1.0
  LANGUAGES CXX
)

if(WIN32)
  message(STATUS "Windows build is enabled (feature parity is in progress).")
elseif(APPLE)
  message(FATAL_ERROR "macOS is not supported currently.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Ensure ABI matches system packages on Linux.
# Some environments default Clang to libc++ (std::__1), while distro libtorrent/boost are built with libstdc++.
if(UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-stdlib=libstdc++)
  add_link_options(-stdlib=libstdc++)
endif()

function(pfd_set_target_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4)
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
  endif()
endfunction()

# Testing
include(CTest)
if(BUILD_TESTING)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
  )
  set(gtest_force_shared_crt OFF CACHE BOOL "" FORCE)
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
endif()

option(BUILD_BENCHMARKS "Build benchmark targets" ON)
if(BUILD_BENCHMARKS)
  include(FetchContent)
  FetchContent_Declare(
    benchmark
    URL https://github.com/google/benchmark/archive/refs/tags/v1.9.1.zip
  )
  set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
  set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(benchmark)
endif()

# Qt: enable automatic MOC/UIC/RCC for Widgets apps.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Qml Quick QuickWidgets)

set(PFD_LIBTORRENT_TARGET "")
find_package(libtorrent-rasterbar CONFIG QUIET)
if(TARGET torrent-rasterbar::torrent-rasterbar)
  set(PFD_LIBTORRENT_TARGET torrent-rasterbar::torrent-rasterbar)
elseif(TARGET libtorrent-rasterbar::libtorrent-rasterbar)
  set(PFD_LIBTORRENT_TARGET libtorrent-rasterbar::libtorrent-rasterbar)
elseif(TARGET libtorrent::torrent-rasterbar)
  set(PFD_LIBTORRENT_TARGET libtorrent::torrent-rasterbar)
endif()

if(PFD_LIBTORRENT_TARGET STREQUAL "" AND UNIX AND NOT APPLE)
  find_package(PkgConfig REQUIRED)
  pkg_check_modules(LIBTORRENT REQUIRED IMPORTED_TARGET libtorrent-rasterbar)
  set(PFD_LIBTORRENT_TARGET PkgConfig::LIBTORRENT)
endif()

if(PFD_LIBTORRENT_TARGET STREQUAL "")
  message(FATAL_ERROR
          "Unable to find libtorrent. On Linux install libtorrent-rasterbar-dev/pkg-config; "
          "on Windows prefer vcpkg and pass CMAKE_TOOLCHAIN_FILE.")
endif()

add_subdirectory(src/base)
add_subdirectory(src/core)
add_subdirectory(src/lt)
add_subdirectory(src/ui)

add_executable(p2p_downloader
  src/app_controller.h
  src/app_controller.cc
  src/main.cc
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc")
  message(FATAL_ERROR "Missing src/main.cc. Create it (Qt app entrypoint) or update add_executable() sources.")
endif()

target_link_libraries(p2p_downloader
  PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${PFD_LIBTORRENT_TARGET}
    p2p_base
    p2p_core
    p2p_lt
    p2p_ui
)

pfd_set_target_warnings(p2p_downloader)

target_compile_definitions(p2p_downloader PRIVATE
  QT_NO_KEYWORDS
)

if(BUILD_TESTING)
  find_package(Qt6 REQUIRED COMPONENTS Test)
  enable_testing()

  add_executable(p2p_tests
    tests/test_main.cc
    tests/test_sanity.cc
    tests/test_base_errors.cc
    tests/test_format.cc
    tests/test_input_sanitizer.cc
    tests/test_io.cc
    tests/test_process_utils.cc
    tests/test_paths.cc
    tests/test_threading.cc
    tests/test_types.cc
    tests/test_clock.cc
    tests/test_fs_utils.cc
    tests/test_system_info.cc
    tests/test_time_stamps.cc
    tests/test_time_format.cc
    tests/test_throttle.cc
    tests/test_debounce.cc
    tests/test_lt_error_conversions.cc
    tests/test_lt_task_event_mapper.cc
    tests/test_lt_task_event_dispatcher.cc
    tests/test_lt_task_alert_adapter.cc
    tests/test_session_worker.cc
    tests/test_ring_buffer.cc
    tests/test_log_store.cc
    tests/test_logger.cc
    tests/test_task_state_store.cc
    tests/test_task_pipeline.cc
    tests/test_task_pipeline_service.cc
    tests/test_rss_parser.cc
    tests/test_rss_repository.cc
    tests/test_rss_service.cc
    tests/test_rss_rule_engine.cc
    tests/test_rss_series_tracker.cc
    tests/test_rss_auto_download.cc
    tests/test_rss_dedup.cc
    tests/test_rss_opml.cc
    tests/test_save_path_policy.cc
    tests/test_task_add_builder.cc
  )

  target_link_libraries(p2p_tests
    PRIVATE
      GTest::gtest
      Qt6::Core
      p2p_base
      p2p_core
      p2p_lt
  )

  pfd_set_target_warnings(p2p_tests)

  include(GoogleTest)
  gtest_discover_tests(p2p_tests)

  add_executable(p2p_qt_tests
    tests/qt/tst_pfd_qt.cpp
  )
  target_link_libraries(p2p_qt_tests
    PRIVATE
      Qt6::Core
      Qt6::Test
      p2p_base
      p2p_lt
  )
  target_include_directories(p2p_qt_tests
    PRIVATE
      ${PROJECT_SOURCE_DIR}/src
  )
  pfd_set_target_warnings(p2p_qt_tests)
  add_test(NAME p2p_qt_tests COMMAND p2p_qt_tests)
endif()

if(BUILD_BENCHMARKS)
  add_executable(bench_logger_qps
    benchmarks/bench_logger_qps.cc
  )
  target_link_libraries(bench_logger_qps
    PRIVATE
      benchmark::benchmark
      Qt6::Core
      p2p_base
      p2p_core
  )
  pfd_set_target_warnings(bench_logger_qps)
endif()
