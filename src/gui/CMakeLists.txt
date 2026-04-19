add_executable(stackcalc-gui WIN32 MACOSX_BUNDLE
    main.cpp
    gui_app.cpp
)
target_link_libraries(stackcalc-gui PRIVATE
    stackcalc-controller
    wx::core
    wx::base
)

# On Windows, embed wxWidgets' default resources (incl. the DPI-aware
# manifest) so the app renders sharply on high-DPI displays instead of
# being bitmap-scaled by the OS.
if(WIN32)
    enable_language(RC)
    target_sources(stackcalc-gui PRIVATE stackcalc.rc)
endif()
