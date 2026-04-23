cmake_minimum_required(VERSION 3.22)

project(torrentlink
  VERSION 0.1.0
  LANGUAGES CXX
)

include(GNUInstallDirs)

if(WIN32)
  message(STATUS "Windows build is enabled (feature parity is in progress).")
  add_compile_definitions(
    _WIN32_WINNT=0x0601
    WINVER=0x0601
    NOMINMAX
  )
elseif(APPLE)
  message(FATAL_ERROR "macOS is not supported currently.")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(PFD_WARNINGS_AS_ERRORS "Treat warnings as errors" OFF)
option(PFD_ENABLE_LTO "Enable link time optimization (IPO/LTO)" OFF)

if(UNIX AND NOT APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
  add_compile_options(-stdlib=libstdc++)
  add_link_options(-stdlib=libstdc++)
endif()

function(pfd_set_target_warnings target_name)
  if(MSVC)
    target_compile_options(${target_name} PRIVATE /W4 /wd4324)
    if(PFD_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    if(PFD_WARNINGS_AS_ERRORS)
      target_compile_options(${target_name} PRIVATE -Werror)
    endif()
  endif()
endfunction()

if(PFD_ENABLE_LTO)
  include(CheckIPOSupported)
  check_ipo_supported(RESULT pfd_ipo_supported OUTPUT pfd_ipo_error)
  if(pfd_ipo_supported)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
  else()
    message(WARNING "PFD_ENABLE_LTO=ON but IPO is not supported: ${pfd_ipo_error}")
  endif()
endif()

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
  find_package(Qt6 REQUIRED COMPONENTS Test)
  enable_testing()
endif()

# Qt: enable automatic MOC/UIC/RCC for Widgets apps.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Widgets Network Qml Quick QuickWidgets)

set(PFD_LIBTORRENT_TARGET "")
find_package(LibtorrentRasterbar CONFIG QUIET)
find_package(libtorrent-rasterbar CONFIG QUIET)
if(TARGET LibtorrentRasterbar::torrent-rasterbar)
  set(PFD_LIBTORRENT_TARGET LibtorrentRasterbar::torrent-rasterbar)
elseif(TARGET torrent-rasterbar::torrent-rasterbar)
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
          "on Windows use vcpkg (port name: libtorrent) and pass CMAKE_TOOLCHAIN_FILE.")
endif()

add_subdirectory(src/base)
add_subdirectory(src/core)
add_subdirectory(src/lt)
add_subdirectory(src/ui)
add_subdirectory(src/app)

add_executable(torrentlink
  src/app_controller.h
  src/app_controller.cc
  src/main.cc
)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc")
  message(FATAL_ERROR "Missing src/main.cc. Create it (Qt app entrypoint) or update add_executable() sources.")
endif()

target_link_libraries(torrentlink
  PRIVATE
    Qt6::Core
    Qt6::Widgets
    Qt6::Network
    ${PFD_LIBTORRENT_TARGET}
    torrentlink_base
    torrentlink_core
    torrentlink_lt
    torrentlink_ui
    torrentlink_app
)

pfd_set_target_warnings(torrentlink)

target_compile_definitions(torrentlink PRIVATE
  QT_NO_KEYWORDS
)

install(TARGETS torrentlink
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
