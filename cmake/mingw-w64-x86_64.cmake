# Cross-compile to 64-bit Windows from Linux using mingw-w64.
#
# Usage:
#   cmake -B build -S . \
#         -DCMAKE_TOOLCHAIN_FILE=cmake/mingw-w64-x86_64.cmake \
#         -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j
#
# Requires: gcc-mingw-w64-x86-64 + g++-mingw-w64-x86-64 (Ubuntu:
#   apt install mingw-w64). The "-posix" variants are pinned
# explicitly because std::thread (used by CalcRunner) needs the POSIX
# runtime — the win32 runtime ships C++ stdlib but stubs out
# std::thread to throw at construction. Ubuntu 22.04+ defaults to
# -posix via update-alternatives, but we don't want to depend on
# whatever the host happens to have selected.

set(CMAKE_SYSTEM_NAME      Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(_TC x86_64-w64-mingw32)

# Probe for -posix-suffixed binaries first (Ubuntu/Debian's
# packaging) and fall back to unsuffixed (Homebrew's, where the
# default thread model is already POSIX). Pinning -posix matters
# because std::thread (CalcRunner) needs the POSIX runtime — the
# win32 runtime stubs std::thread to throw.
find_program(_MINGW_GCC NAMES ${_TC}-gcc-posix ${_TC}-gcc REQUIRED)
find_program(_MINGW_GXX NAMES ${_TC}-g++-posix ${_TC}-g++ REQUIRED)
find_program(_MINGW_RC  NAMES ${_TC}-windres        REQUIRED)
set(CMAKE_C_COMPILER   "${_MINGW_GCC}")
set(CMAKE_CXX_COMPILER "${_MINGW_GXX}")
set(CMAKE_RC_COMPILER  "${_MINGW_RC}")

# Confine find_{file,library,include,package} to the mingw sysroot
# so we don't accidentally pick up host (Linux/macOS) headers and
# libs. Both the Debian/Ubuntu layout (/usr/${_TC}) and Homebrew's
# (/opt/homebrew/.../${_TC}) are covered.
get_filename_component(_MINGW_BIN "${_MINGW_GCC}" DIRECTORY)
get_filename_component(_MINGW_PREFIX "${_MINGW_BIN}" DIRECTORY)
set(CMAKE_FIND_ROOT_PATH "${_MINGW_PREFIX}/${_TC}" "/usr/${_TC}")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
