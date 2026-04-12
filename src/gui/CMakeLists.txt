add_executable(stackcalc-gui
    main.cpp
    gui_app.cpp
)
target_link_libraries(stackcalc-gui PRIVATE stackcalc-controller imgui)
