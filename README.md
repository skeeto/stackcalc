# stackcalc

A GUI calculator closely modeled after [Emacs `M-x calc`][orig]. RPN,
arbitrary-precision integers/fractions/floats, with the same dense
keystroke vocabulary.

![](assets/screenshot.png)

## Download

Pre-built binaries for macOS (Apple Silicon) and Windows (x64) are on
the [releases page][releases]. Linux users build from source (below).

## Usage

See [manual.md](manual.md) for the full keystroke reference. The
short version: type numbers and press `RET` to push, then operators
(`+`, `-`, `*`, `/`, `^`, etc.) consume from the top of the stack.
`U` undoes, `D` redoes, `Ctrl+G` cancels a long calculation.

## Build from source

    $ cmake -B build
    $ cmake --build build

Dependencies (GMP, MPFR, wxWidgets, GoogleTest) are downloaded
automatically by CMake. The executable lands at
`build/bin/stackcalc.app` (macOS) or `build/bin/stackcalc[.exe]`.

## License

The calculator itself is in the public domain (see UNLICENSE). It
depends on GMP (LGPL), MPFR (LGPL), and wxWidgets (wxWindows
Library Licence — LGPL with a derived-works exception).


[orig]: https://www.gnu.org/software/emacs/manual/html_mono/calc.html
[releases]: https://github.com/skeeto/stackcalc/releases

