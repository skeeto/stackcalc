add_executable(stackcalc-gui WIN32 MACOSX_BUNDLE
    main.cpp
    gui_app.cpp
)
target_link_libraries(stackcalc-gui PRIVATE
    stackcalc-controller
    wx::core
    wx::base
)

# On Windows, embed our own DPI-aware application manifest so Windows
# stops bitmap-scaling the window on high-DPI displays.
if(WIN32)
    enable_language(RC)
    target_sources(stackcalc-gui PRIVATE stackcalc.rc)
    # Make the RC compiler find stackcalc.manifest (referenced by name
    # in stackcalc.rc) regardless of where the build is invoked from,
    # and rebuild the .rc.obj when the manifest changes.
    set_source_files_properties(stackcalc.rc PROPERTIES
        INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}"
        OBJECT_DEPENDS      "${CMAKE_CURRENT_SOURCE_DIR}/stackcalc.manifest"
    )
endif()
