add_executable(stackcalc-gui WIN32 MACOSX_BUNDLE
    main.cpp
    gui_app.cpp
)
target_link_libraries(stackcalc-gui PRIVATE
    stackcalc-controller
    wx::core
    wx::base
)
