can_bridge_add_legacy_sim(can_bridge_legacy_sim_stub "${CAN_BRIDGE_STUB_SIM_INCLUDE_DIR}")

add_executable(can_bridge_tests
  bounded_queue_test.cc
  bridge_config_test.cc
  bridge_test.cc
  can_channel_test.cc
  ini_file_test.cc
  input_scheduler_test.cc
  key_tables_test.cc
  legacy_simulator_test.cc
  output_encoder_test.cc
  overhead_decoder_test.cc
  panel_decoder_test.cc
  rotary_switch_test.cc
)
target_include_directories(can_bridge_tests PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
target_link_libraries(can_bridge_tests PRIVATE
  can_bridge_core can_bridge_legacy_sim_stub GTest::gtest GTest::gtest_main)
can_bridge_configure_target(can_bridge_tests)
if(NOT CMAKE_CROSSCOMPILING)
  # Lets a test check that config/can_bridge.ini documents the defaults.
  target_compile_definitions(can_bridge_tests PRIVATE
    CAN_BRIDGE_SOURCE_DIR="${PROJECT_SOURCE_DIR}")
endif()

if(WIN32)
  # The Windows-only code has no unit tests (it needs the board), but it is
  # compiled here so that every build checks it.
  target_link_libraries(can_bridge_tests PRIVATE can_bridge_advantech)

  # Same for the executable: build it against the stub headers. It refuses
  # to run, see main.cc.
  add_executable(can_bridge_stub_layout_check
    ../src/app/main.cc
    ../src/app/windows_runtime.cc
    ../src/sim/shared_memory_region.cc)
  target_link_libraries(can_bridge_stub_layout_check PRIVATE
    can_bridge_core can_bridge_advantech can_bridge_legacy_sim_stub winmm)
  can_bridge_configure_target(can_bridge_stub_layout_check)
endif()

include(GoogleTest)
if(CMAKE_CROSSCOMPILING AND NOT CMAKE_CROSSCOMPILING_EMULATOR)
  # Tests cannot run on the build host; build them only.
else()
  gtest_discover_tests(can_bridge_tests DISCOVERY_MODE PRE_TEST)
endif()
