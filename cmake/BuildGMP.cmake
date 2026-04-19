include(ExternalProject)

set(GMP_VERSION "6.3.0")
set(GMP_PREFIX "${CMAKE_BINARY_DIR}/_deps/gmp")

ExternalProject_Add(gmp_external
    URL "https://gmplib.org/download/gmp/gmp-${GMP_VERSION}.tar.xz"
    URL_HASH SHA256=a3c2b80201b89e68616f4ad30bc66aee4927c3ce50e33929ca819d5c43538898
    PREFIX "${GMP_PREFIX}"
    CONFIGURE_COMMAND
        sh <SOURCE_DIR>/configure
        --prefix=<INSTALL_DIR>
        --enable-cxx
        --disable-shared
        --with-pic
        "CC=${CMAKE_C_COMPILER}"
        "CXX=${CMAKE_CXX_COMPILER}"
        "CFLAGS=-O2 -std=gnu17"
    BUILD_COMMAND make -j${SC_BUILD_JOBS}
    INSTALL_COMMAND make install
    BUILD_BYPRODUCTS
        "${GMP_PREFIX}/lib/libgmp.a"
        "${GMP_PREFIX}/lib/libgmpxx.a"
)

# Create include dir so CMake doesn't complain at configure time
file(MAKE_DIRECTORY "${GMP_PREFIX}/include")

# Create imported targets
add_library(gmp::gmp STATIC IMPORTED GLOBAL)
set_target_properties(gmp::gmp PROPERTIES
    IMPORTED_LOCATION "${GMP_PREFIX}/lib/libgmp.a"
    INTERFACE_INCLUDE_DIRECTORIES "${GMP_PREFIX}/include"
)
add_dependencies(gmp::gmp gmp_external)

add_library(gmp::gmpxx STATIC IMPORTED GLOBAL)
set_target_properties(gmp::gmpxx PROPERTIES
    IMPORTED_LOCATION "${GMP_PREFIX}/lib/libgmpxx.a"
    INTERFACE_INCLUDE_DIRECTORIES "${GMP_PREFIX}/include"
    INTERFACE_LINK_LIBRARIES gmp::gmp
)
add_dependencies(gmp::gmpxx gmp_external)
