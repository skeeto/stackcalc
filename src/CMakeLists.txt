add_subdirectory(core)
add_subdirectory(calc)
add_subdirectory(controller)

if(STACKCALC_BUILD_GUI)
    add_subdirectory(gui)
endif()
