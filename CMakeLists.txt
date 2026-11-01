cmake_minimum_required(VERSION 3.21)

project(can_bridge
  VERSION 2.0.0
  DESCRIPTION "Bridge between the cockpit CAN controllers and the simulator"
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(CAN_BRIDGE_BUILD_TESTS "Build the unit tests" ON)
option(CAN_BRIDGE_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
set(CAN_BRIDGE_SIM_INCLUDE_DIR "" CACHE PATH
    "Directory with the simulator headers: so2010struct.h, STRUCT_AVN_410_RTSH_4.h, Struct_SUT_410.h")

set(CAN_BRIDGE_STUB_SIM_INCLUDE_DIR "${PROJECT_SOURCE_DIR}/tests/sim_headers_stub")

# --- Common compiler settings -------------------------------------------------

function(can_bridge_configure_target target)
  if(MSVC)  # MSVC and clang-cl.
    target_compile_options(${target} PRIVATE /W4 /permissive- /utf-8 /EHsc)
    if(CAN_BRIDGE_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE /WX)
    endif()
  else()
    target_compile_options(${target} PRIVATE
      -Wall -Wextra -Wpedantic -Wshadow -Wnon-virtual-dtor -Wcast-align)
    if(CAN_BRIDGE_WARNINGS_AS_ERRORS)
      target_compile_options(${target} PRIVATE -Werror)
    endif()
  endif()
  if(WIN32)
    target_compile_definitions(${target} PRIVATE
      WIN32_LEAN_AND_MEAN NOMINMAX _WIN32_WINNT=0x0601)
  endif()
endfunction()

find_package(Threads REQUIRED)

# --- Portable core ------------------------------------------------------------

add_library(can_bridge_core STATIC
  src/app/bridge.cc
  src/app/can_channel.cc
  src/can/can_frame.cc
  src/cockpit/input_fields.cc
  src/cockpit/input_scheduler.cc
  src/common/logger.cc
  src/config/bridge_config.cc
  src/config/ini_file.cc
  src/protocol/key_tables.cc
  src/protocol/output_encoder.cc
  src/protocol/overhead_decoder.cc
  src/protocol/panel_decoder.cc
  src/protocol/rotary_switch.cc
)
target_include_directories(can_bridge_core PUBLIC "${PROJECT_SOURCE_DIR}/src")
target_link_libraries(can_bridge_core PUBLIC Threads::Threads)
can_bridge_configure_target(can_bridge_core)

# Adapter for the legacy shared-memory layout. It needs the simulator headers,
# so it is built once per header directory (real headers, test stubs).
function(can_bridge_add_legacy_sim target include_dir)
  add_library(${target} STATIC "${PROJECT_SOURCE_DIR}/src/sim/legacy_simulator.cc")
  target_include_directories(${target} PUBLIC "${include_dir}")
  target_link_libraries(${target} PUBLIC can_bridge_core)
  can_bridge_configure_target(${target})
endfunction()

# --- Windows executable -------------------------------------------------------

if(WIN32)
  add_library(can_bridge_advantech STATIC
    src/can/advantech/advantech_can_port.cc)
  target_link_libraries(can_bridge_advantech PUBLIC can_bridge_core)
  can_bridge_configure_target(can_bridge_advantech)

  if(CAN_BRIDGE_SIM_INCLUDE_DIR)
    can_bridge_add_legacy_sim(can_bridge_legacy_sim "${CAN_BRIDGE_SIM_INCLUDE_DIR}")

    add_executable(can_bridge
      src/app/main.cc
      src/app/windows_runtime.cc
      src/sim/shared_memory_region.cc)
    target_link_libraries(can_bridge PRIVATE
      can_bridge_core can_bridge_advantech can_bridge_legacy_sim winmm)
    target_compile_definitions(can_bridge PRIVATE
      CAN_BRIDGE_VERSION="${PROJECT_VERSION}")
    can_bridge_configure_target(can_bridge)
    if(MINGW)
      # Self-contained exe: no libstdc++/libgcc/winpthread DLLs to deploy.
      target_link_options(can_bridge PRIVATE -static)
    endif()

    install(TARGETS can_bridge RUNTIME DESTINATION .)
    install(FILES config/can_bridge.ini DESTINATION .)
  else()
    message(WARNING
      "CAN_BRIDGE_SIM_INCLUDE_DIR is not set: the can_bridge executable is "
      "skipped. Point it at the simulator headers to build it.")
  endif()
endif()

# --- Tests --------------------------------------------------------------------

if(CAN_BRIDGE_BUILD_TESTS)
  enable_testing()
  find_package(GTest QUIET)
  if(NOT GTest_FOUND)
    include(FetchContent)
    FetchContent_Declare(googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.15.2
      GIT_SHALLOW TRUE)
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
  endif()
  add_subdirectory(tests)
endif()
