cmake_minimum_required(VERSION 3.24)
project(stackcalc VERSION 1.0.0 LANGUAGES C CXX)

option(STACKCALC_USE_CCACHE "Use ccache to speed up rebuilds if available" ON)
if(STACKCALC_USE_CCACHE)
    find_program(CCACHE_PROGRAM ccache)
    if(CCACHE_PROGRAM)
        message(STATUS "ccache found: ${CCACHE_PROGRAM}")
        set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
        set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    endif()
endif()

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

# Force UTF-8 source AND execution charsets on MSVC. By default MSVC
# picks the source charset based on the system locale, which on most
# Windows machines is NOT UTF-8 — so our em-dashes and other non-ASCII
# literals get reinterpreted at compile time and emerge as mojibake at
# run time. Apply only to our own targets (placed AFTER FetchWxWidgets
# so wx's bundled C/C++ deps build with their own settings).
if(MSVC)
    add_compile_options(/utf-8)
endif()

add_subdirectory(src)

enable_testing()
include(cmake/FetchGoogleTest.cmake)
add_subdirectory(tests)

# --- Packaging (CPack) ---
# `cmake --build build --target package` produces a release artifact:
#   macOS:   stackcalc-<ver>-macos-arm64.dmg   (drag-to-Applications)
#   Windows: stackcalc-<ver>-windows-x64.zip   (stackcalc.exe + manual.md)
set(CPACK_PACKAGE_NAME "stackcalc")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
if(APPLE)
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_PACKAGE_FILE_NAME "stackcalc-${PROJECT_VERSION}-macos-arm64")
    set(CPACK_DMG_VOLUME_NAME "stackcalc")
elseif(WIN32)
    set(CPACK_GENERATOR "ZIP")
    set(CPACK_PACKAGE_FILE_NAME "stackcalc-${PROJECT_VERSION}-windows-x64")
    install(FILES manual.md DESTINATION .)
endif()
include(CPack)
