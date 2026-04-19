cmake_minimum_required(VERSION 3.24)
project(stackcalc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Parallelism for ExternalProject native builds (GMP/MPFR run their own
# autotools 'make', which doesn't see CMake's job server). Honor the
# standard env var if set; otherwise fall back to detected core count.
if(DEFINED ENV{CMAKE_BUILD_PARALLEL_LEVEL})
    set(SC_BUILD_JOBS $ENV{CMAKE_BUILD_PARALLEL_LEVEL})
else()
    include(ProcessorCount)
    ProcessorCount(SC_BUILD_JOBS)
    if(SC_BUILD_JOBS EQUAL 0)
        set(SC_BUILD_JOBS 4)
    endif()
endif()

include(cmake/BuildGMP.cmake)
include(cmake/BuildMPFR.cmake)

option(STACKCALC_BUILD_GUI "Build the wxWidgets GUI frontend" ON)
if(STACKCALC_BUILD_GUI)
    include(cmake/FetchWxWidgets.cmake)
endif()

add_subdirectory(src)

enable_testing()
include(cmake/FetchGoogleTest.cmake)
add_subdirectory(tests)
