if(DEPS STREQUAL "LOCAL")
    # find_package(wxWidgets) has two resolution paths:
    #
    #   CONFIG mode — finds wxWidgetsConfig.cmake shipped by a CMake-
    #     built wxWidgets install. Creates modern imported targets
    #     wx::core / wx::base / wx::richtext directly, no
    #     wxWidgets_USE_FILE. This is what works under w64devkit /
    #     mingw on Windows where wx-config isn't a usable shell
    #     script.
    #
    #   MODULE mode — uses CMake's bundled FindwxWidgets.cmake, which
    #     shells out to wx-config and returns variables (LIBRARIES,
    #     INCLUDE_DIRS, USE_FILE) instead of targets. This is the
    #     typical Linux distro path (apt install libwxgtk3.2-dev) and
    #     also what Homebrew installs on macOS.
    #
    # Try CONFIG first (modern, works everywhere wx ships a Config
    # file), fall back to MODULE (wx-config). The `if(wxWidgets_USE_FILE)`
    # guard below discriminates: in CONFIG mode the targets already
    # exist and there's nothing for us to do; in MODULE mode we wrap
    # the variables in interface targets so the rest of the build can
    # name wx::core / wx::base / wx::richtext uniformly with the
    # FETCH path.
    find_package(wxWidgets 3.2 QUIET CONFIG
        COMPONENTS core base richtext)
    if(NOT wxWidgets_FOUND)
        find_package(wxWidgets 3.2 REQUIRED MODULE
            COMPONENTS core base richtext)
    endif()
    if(wxWidgets_USE_FILE)
        # MODULE mode result: include the use-file (sets compile
        # flags / definitions globally — fine for a single-app
        # build) and synthesise the wx::* targets from the
        # combined link list. All three aliases point at the same
        # libraries; the linker dedupes.
        include(${wxWidgets_USE_FILE})
        foreach(_comp core base richtext)
            if(NOT TARGET wx::${_comp})
                add_library(wx::${_comp} INTERFACE IMPORTED)
                target_link_libraries(wx::${_comp}
                    INTERFACE ${wxWidgets_LIBRARIES})
            endif()
        endforeach()
    endif()
    return()
endif()

include(FetchContent)

# Must be set BEFORE FetchContent_MakeAvailable
set(wxBUILD_SHARED  OFF CACHE BOOL "" FORCE)  # static — simpler on macOS
set(wxBUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxBUILD_DEMOS   OFF CACHE BOOL "" FORCE)
set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(wxUSE_STC       OFF CACHE BOOL "" FORCE)  # skip Scintilla
set(wxUSE_WEBVIEW   OFF CACHE BOOL "" FORCE)  # skip WebKit on macOS

# The release tarball includes all bundled third-party sources inline (zlib,
# libpng, libjpeg, libtiff, expat, etc.), so unlike the git repo there are no
# submodules to recurse.
FetchContent_Declare(
    wxWidgets
    URL      https://github.com/wxWidgets/wxWidgets/releases/download/v3.2.10/wxWidgets-3.2.10.tar.bz2
    URL_HASH SHA256=d66e929569947a4a5920699539089a9bda83a93e5f4917fb313a61f0c344b896
)
FetchContent_MakeAvailable(wxWidgets)
