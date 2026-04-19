include(ExternalProject)

set(MPFR_VERSION "4.2.1")
set(MPFR_PREFIX "${CMAKE_BINARY_DIR}/_deps/mpfr")
set(GMP_PREFIX "${CMAKE_BINARY_DIR}/_deps/gmp")

ExternalProject_Add(mpfr_external
    URL "https://www.mpfr.org/mpfr-${MPFR_VERSION}/mpfr-${MPFR_VERSION}.tar.xz"
    URL_HASH SHA256=277807353a6726978996945af13e52829e3abd7a9a5b7fb2793894e18f1fcbb2
    PREFIX "${MPFR_PREFIX}"
    DEPENDS gmp_external
    CONFIGURE_COMMAND
        sh <SOURCE_DIR>/configure
        --prefix=<INSTALL_DIR>
        --disable-shared
        --with-pic
        --with-gmp=${GMP_PREFIX}
        "CC=${CMAKE_C_COMPILER}"
        "CFLAGS=-O2 -std=gnu17"
    BUILD_COMMAND make -j${SC_BUILD_JOBS}
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS
        "${MPFR_PREFIX}/lib/libmpfr.a"
)

file(MAKE_DIRECTORY "${MPFR_PREFIX}/include")

add_library(mpfr::mpfr STATIC IMPORTED GLOBAL)
set_target_properties(mpfr::mpfr PROPERTIES
    IMPORTED_LOCATION "${MPFR_PREFIX}/lib/libmpfr.a"
    INTERFACE_INCLUDE_DIRECTORIES "${MPFR_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES gmp::gmp
)
add_dependencies(mpfr::mpfr mpfr_external)
