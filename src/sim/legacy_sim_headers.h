#ifndef CAN_BRIDGE_SIM_LEGACY_SIM_HEADERS_H_
#define CAN_BRIDGE_SIM_LEGACY_SIM_HEADERS_H_

// Shared-memory structures of the simulator (SHAVN_410, SHSUT).
//
// These headers belong to the simulator project and are intentionally not
// part of this repository. Point the CMake cache variable
// CAN_BRIDGE_SIM_INCLUDE_DIR at the directory that contains them. The include
// order is the one the legacy program used.

#ifdef _WIN32
#include <windows.h>  // The simulator headers rely on Win32 types.
#endif

// clang-format off
#include <so2010struct.h>
#include <STRUCT_AVN_410_RTSH_4.h>
#include <Struct_SUT_410.h>
// clang-format on

#endif  // CAN_BRIDGE_SIM_LEGACY_SIM_HEADERS_H_
