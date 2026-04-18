cmake_minimum_required(VERSION 3.20)
project(stackcalc VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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
