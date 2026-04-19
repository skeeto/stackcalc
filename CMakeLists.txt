cmake_minimum_required(VERSION 3.24)
project(stackcalc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Pin C to gnu17 so wxWidgets' bundled C deps (zlib, libpng, libjpeg,
# libtiff, expat) don't trip GCC's C23 default and emit a flood of
# -Wold-style-definition warnings on their K&R-era function signatures.
# We have no hand-written C in this project; CMAKE_C_EXTENSIONS defaults
# to ON, which gives -std=gnu17.
set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/FetchGMP.cmake)
include(cmake/FetchMPFR.cmake)

option(STACKCALC_BUILD_GUI "Build the wxWidgets GUI frontend" ON)
if(STACKCALC_BUILD_GUI)
    include(cmake/FetchWxWidgets.cmake)
endif()

add_subdirectory(src)

enable_testing()
include(cmake/FetchGoogleTest.cmake)
add_subdirectory(tests)
