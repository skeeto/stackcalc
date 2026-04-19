add_executable(stackcalc WIN32 MACOSX_BUNDLE
    main.cpp
    gui_app.cpp
)
target_link_libraries(stackcalc PRIVATE
    stackcalc-controller
    wx::core
    wx::base
    wx::richtext
)

# Generate version.hpp from version.hpp.in so the C++ code, the
# Windows manifest, and the macOS bundle Info.plist all read from
# project(... VERSION ...) at the top of the root CMakeLists.
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in"
    "${CMAKE_CURRENT_BINARY_DIR}/version.hpp"
    @ONLY
)
target_include_directories(stackcalc PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

# Drop the executable (or .app bundle on macOS) into build/bin/ instead
# of build/src/gui/. Test executables stay where they are.
# MACOSX_BUNDLE_* properties feed into the default Info.plist template
# CMake stamps out for bundle targets.
set_target_properties(stackcalc PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY            "${CMAKE_BINARY_DIR}/bin"
    MACOSX_BUNDLE_BUNDLE_NAME            "stackcalc"
    MACOSX_BUNDLE_BUNDLE_VERSION         "${PROJECT_VERSION}"
    MACOSX_BUNDLE_SHORT_VERSION_STRING   "${PROJECT_VERSION}"
    MACOSX_BUNDLE_GUI_IDENTIFIER         "stackcalc"
    MACOSX_BUNDLE_INFO_STRING            "stackcalc ${PROJECT_VERSION}"
)

# Install rules for CPack packaging. On Windows the .exe lands at the
# root of the ZIP; on macOS the .app bundle lands at the root of the DMG.
install(TARGETS stackcalc
    RUNTIME DESTINATION .
    BUNDLE  DESTINATION .
)

# On Windows, embed our own DPI-aware application manifest so Windows
# stops bitmap-scaling the window on high-DPI displays. The manifest
# is generated from stackcalc.manifest.in so the assemblyIdentity
# version tracks PROJECT_VERSION.
if(WIN32)
    enable_language(RC)
    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/stackcalc.manifest.in"
        "${CMAKE_CURRENT_BINARY_DIR}/stackcalc.manifest"
        @ONLY
    )
    target_sources(stackcalc PRIVATE stackcalc.rc)
    # The .rc references stackcalc.manifest by name; tell the RC
    # compiler to look in the build dir (where configure_file landed
    # the generated copy), and re-link the .rc.obj when it changes.
    set_source_files_properties(stackcalc.rc PROPERTIES
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR}"
        OBJECT_DEPENDS      "${CMAKE_CURRENT_BINARY_DIR}/stackcalc.manifest"
    )
endif()
