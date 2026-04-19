include(FetchContent)

FetchContent_Declare(
    mpfr
    URL      https://github.com/skeeto/mpfr-cmake/archive/refs/tags/v4.2.2.tar.gz
    URL_HASH SHA256=f60bf3bfc003fe682e2fc3ada25c66032db979d3b9bca4ceb8ab25166cf0af87
)
FetchContent_MakeAvailable(mpfr)
