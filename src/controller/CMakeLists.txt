add_library(stackcalc-controller STATIC
    key_event.cpp
    input_state.cpp
    key_map.cpp
    controller.cpp
)
target_include_directories(stackcalc-controller PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(stackcalc-controller PUBLIC stackcalc-calc)
