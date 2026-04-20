if(DEPS STREQUAL "LOCAL")
    # System libgmp / libgmpxx. The headers and libraries are looked
    # up via find_path / find_library so this works on any sensibly
    # configured Unix-like system; CMake's defaults search /usr/{lib,
    # include}, /usr/local, and Homebrew prefixes.
    find_path(GMP_INCLUDE_DIR     NAMES gmp.h   REQUIRED)
    find_library(GMP_LIBRARY      NAMES gmp     REQUIRED)
    find_library(GMPXX_LIBRARY    NAMES gmpxx   REQUIRED)
    if(NOT TARGET GMP::gmp)
        add_library(GMP::gmp UNKNOWN IMPORTED)
        set_target_properties(GMP::gmp PROPERTIES
            IMPORTED_LOCATION             "${GMP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}")
    endif()
    if(NOT TARGET GMP::gmpxx)
        add_library(GMP::gmpxx UNKNOWN IMPORTED)
        set_target_properties(GMP::gmpxx PROPERTIES
            IMPORTED_LOCATION             "${GMPXX_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${GMP_INCLUDE_DIR}"
            INTERFACE_LINK_LIBRARIES      GMP::gmp)
    endif()
    return()
endif()

include(FetchContent)

FetchContent_Declare(
    gmp
    URL      https://github.com/skeeto/gmp-cmake/archive/refs/tags/v6.3.0.1.tar.gz
    URL_HASH SHA256=de8b6262c05303705873f4e07333f22869d220818b67b95e8e93c8889b6eca2f
)
FetchContent_MakeAvailable(gmp)

# Enable C++ exception unwinding through GMP's C frames. We install a
# custom allocator (mp_set_memory_functions) that throws CancelledException
# on user cancel; that exception has to propagate up through whatever
# GMP function called the allocator. Without exception-table support
# in the C compilation, the unwinder hits a frame with no FDE and
# either terminates or skips RAII destructors above it.
#   -fexceptions: gcc/clang/mingw — emits .eh_frame for C functions.
#   /EHa:         MSVC — asynchronous exception model; lets C++
#                 exceptions cross C frames via SEH unwind.
foreach(_t IN ITEMS gmp gmpxx)
    if(TARGET ${_t})
        if(MSVC)
            target_compile_options(${_t} PRIVATE /EHa)
        else()
            target_compile_options(${_t} PRIVATE -fexceptions)
        endif()
    endif()
endforeach()
