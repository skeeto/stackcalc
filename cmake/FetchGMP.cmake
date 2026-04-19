include(FetchContent)

FetchContent_Declare(
    gmp
    URL      https://github.com/skeeto/gmp-cmake/archive/refs/tags/v6.3.0.tar.gz
    URL_HASH SHA256=599bf23e44176c813892b6dfb7b60957041e69b7f4f25144393501f7ca879d85
)
FetchContent_MakeAvailable(gmp)
