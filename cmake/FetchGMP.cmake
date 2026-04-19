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
