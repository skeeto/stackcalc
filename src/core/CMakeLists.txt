add_library(stackcalc-core STATIC
    value.cpp
    integer.cpp
    fraction.cpp
    decimal_float.cpp
    coercion.cpp
    arithmetic.cpp
    complex.cpp
    hms.cpp
    error_form.cpp
    modulo_form.cpp
    interval.cpp
    infinity.cpp
    vector.cpp
    formatter.cpp
    bitwise.cpp
    mpfr_bridge.cpp
    constants.cpp
    scientific.cpp
)
target_include_directories(stackcalc-core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
target_link_libraries(stackcalc-core PUBLIC GMP::gmpxx MPFR::mpfr)
