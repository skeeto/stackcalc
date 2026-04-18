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
