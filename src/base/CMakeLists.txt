add_library(torrentlink_base STATIC
  clock.h
  clock.cc
  ring_buffer.h
  fs_utils.h
  fs_utils.cc
  system_info.h
  system_info.cc
  errors.h
  errors.cc
  process_utils.h
  process_utils.cc
  format.h
  format.cc
  input_sanitizer.h
  input_sanitizer.cc
  io.h
  io.cc
  paths.h
  paths.cc
  threading.h
  threading.cc
  types.h
  time_stamps.h
  time_stamps.cc
  time_format.h
  time_format.cc
  throttle.h
  throttle.cc
  debounce.h
  debounce.cc
)

target_include_directories(torrentlink_base
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
)

target_link_libraries(torrentlink_base
  PUBLIC
    Qt6::Core
)

target_compile_features(torrentlink_base PUBLIC cxx_std_17)
pfd_set_target_warnings(torrentlink_base)

