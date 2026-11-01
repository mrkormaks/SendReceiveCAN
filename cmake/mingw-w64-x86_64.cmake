# Cross-compiles the Windows executable on Linux with MinGW-w64, e.g. on
# Ubuntu: apt install g++-mingw-w64-x86-64-posix
#
#   cmake --preset mingw-cross -DCAN_BRIDGE_SIM_INCLUDE_DIR=/path/to/headers
#
# Pass -DCMAKE_CROSSCOMPILING_EMULATOR=wine to run the tests through Wine.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(_triplet x86_64-w64-mingw32)
# The "-posix" variant provides std::thread on top of winpthreads. The C
# compiler is needed by third-party projects such as GoogleTest.
find_program(CMAKE_CXX_COMPILER NAMES ${_triplet}-g++-posix ${_triplet}-g++ REQUIRED)
find_program(CMAKE_C_COMPILER NAMES ${_triplet}-gcc-posix ${_triplet}-gcc REQUIRED)
find_program(CMAKE_RC_COMPILER NAMES ${_triplet}-windres)

set(CMAKE_FIND_ROOT_PATH /usr/${_triplet})
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
