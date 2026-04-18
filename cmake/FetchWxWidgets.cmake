include(FetchContent)

# Must be set BEFORE FetchContent_MakeAvailable
set(wxBUILD_SHARED  OFF CACHE BOOL "" FORCE)  # static — simpler on macOS
set(wxBUILD_TESTS   OFF CACHE BOOL "" FORCE)
set(wxBUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(wxBUILD_DEMOS   OFF CACHE BOOL "" FORCE)
set(wxBUILD_INSTALL OFF CACHE BOOL "" FORCE)
set(wxUSE_STC       OFF CACHE BOOL "" FORCE)  # skip Scintilla/Lexilla submodules
set(wxUSE_WEBVIEW   OFF CACHE BOOL "" FORCE)  # skip WebKit on macOS

FetchContent_Declare(
    wxWidgets
    GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
    GIT_TAG        v3.2.10
    GIT_SHALLOW    TRUE
    GIT_SUBMODULES_RECURSE TRUE   # CRITICAL — vendored deps live in submodules
)
FetchContent_MakeAvailable(wxWidgets)
