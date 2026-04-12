add_library(stackcalc-calc STATIC
    stack.cpp
    trail.cpp
    undo.cpp
    operations.cpp
    variables.cpp
)
target_include_directories(stackcalc-calc PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(stackcalc-calc PUBLIC stackcalc-core)
