if(DEPS STREQUAL "LOCAL")
    find_path(MPFR_INCLUDE_DIR  NAMES mpfr.h  REQUIRED)
    find_library(MPFR_LIBRARY   NAMES mpfr    REQUIRED)
    if(NOT TARGET MPFR::mpfr)
        add_library(MPFR::mpfr UNKNOWN IMPORTED)
        set_target_properties(MPFR::mpfr PROPERTIES
            IMPORTED_LOCATION             "${MPFR_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${MPFR_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES      GMP::gmp)
    endif()
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    mpfr
    URL      https://github.com/skeeto/mpfr-cmake/archive/refs/tags/v4.2.2.tar.gz
    URL_HASH SHA256=f60bf3bfc003fe682e2fc3ada25c66032db979d3b9bca4ceb8ab25166cf0af87
)
FetchContent_MakeAvailable(mpfr)

# See FetchGMP.cmake for the rationale. MPFR queries GMP's allocator
# on every alloc (mpfr-gmp.c), so our cancellation throw originates
# inside GMP's hook but unwinds through MPFR's C frames before
# reaching our C++ caller — needs the same exception-table flag.
if(TARGET mpfr)
    if(MSVC)
        target_compile_options(mpfr PRIVATE /EHa)
    else()
        target_compile_options(mpfr PRIVATE -fexceptions)
    endif()
endif()
