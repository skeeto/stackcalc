add_executable(stackcalc WIN32 MACOSX_BUNDLE
    main.cpp
    gui_app.cpp
)
target_link_libraries(stackcalc PRIVATE
    stackcalc-controller
    wx::core
    wx::base
)

# Statically link libstdc++ / libgcc / winpthreads on mingw so the
# released stackcalc.exe has no .dll dependencies beyond the system
# Windows libraries. Without this users would need the matching
# libstdc++-6.dll / libgcc_s_seh-1.dll / libwinpthread-1.dll
# alongside the EXE — fragile and confusing.
if(MINGW)
    target_link_options(stackcalc PRIVATE
        -static-libgcc
        -static-libstdc++
        -static  # also picks up libwinpthread (needed for std::thread)
    )
endif()

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
    MACOSX_BUNDLE_GUI_IDENTIFIER         "com.nullprogram.stackcalc"
    MACOSX_BUNDLE_INFO_STRING            "stackcalc ${PROJECT_VERSION}"
    MACOSX_BUNDLE_ICON_FILE              "icon.icns"
)

# Bundle the .icns into App.app/Contents/Resources/ on macOS. The
# MACOSX_BUNDLE_ICON_FILE property above tells Info.plist's
# CFBundleIconFile to look for it there. Adding the file to
# target_sources is what triggers the copy.
if(APPLE)
    set(_icon_icns "${CMAKE_SOURCE_DIR}/assets/icon.icns")
    set_source_files_properties("${_icon_icns}" PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(stackcalc PRIVATE "${_icon_icns}")
endif()

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
    # The .rc references stackcalc.manifest (in the build dir, from
    # configure_file above) and icon.ico (in the source assets dir).
    # Both directories go on the RC compiler's include path; both
    # files go in OBJECT_DEPENDS so the .rc.obj rebuilds when
    # either changes.
    set_source_files_properties(stackcalc.rc PROPERTIES
        INCLUDE_DIRECTORIES
            "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_SOURCE_DIR}/assets"
        OBJECT_DEPENDS
            "${CMAKE_CURRENT_BINARY_DIR}/stackcalc.manifest;${CMAKE_SOURCE_DIR}/assets/icon.ico"
    )
endif()
