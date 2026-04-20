if(DEPS STREQUAL "LOCAL")
    # System wxWidgets via wx-config (Linux distros, Homebrew on macOS).
    # FindwxWidgets is bundled with CMake. It returns ${wxWidgets_LIBRARIES}
    # as a single combined link line for all requested components, plus
    # ${wxWidgets_INCLUDE_DIRS} / ${wxWidgets_DEFINITIONS} / ${wxWidgets_CXX_FLAGS}.
    # ${wxWidgets_USE_FILE} bundles those into a single include(...) that
    # sets things globally — fine for a single-app build like ours.
    find_package(wxWidgets 3.2 REQUIRED COMPONENTS core base richtext)
    include(${wxWidgets_USE_FILE})
    # Wrap the find result in modern-style imported targets so the rest
    # of the build can name them as wx::core / wx::base / wx::richtext
    # uniformly with the FETCH path. All three aliases point at the
    # same combined link list — FindwxWidgets doesn't separate per-
    # component, and the linker will dedupe.
    foreach(_comp core base richtext)
        if(NOT TARGET wx::${_comp})
            add_library(wx::${_comp} INTERFACE IMPORTED)
            target_link_libraries(wx::${_comp}
                INTERFACE ${wxWidgets_LIBRARIES})
        endif()
    endforeach()
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
