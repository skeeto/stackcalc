#pragma once

#include <string>

namespace sc {

enum class AngularMode { Degrees, Radians };
enum class DisplayFormat { Normal, Fix, Sci, Eng };
enum class FractionMode { Float, Fraction };
enum class ComplexFormat { Pair, INotation, JNotation };

struct CalcState {
    int precision = 12;
    AngularMode angular_mode = AngularMode::Degrees;
    DisplayFormat display_format = DisplayFormat::Normal;
    int display_digits = -1;      // for Fix/Sci/Eng; -1 = use precision
    int display_radix = 10;
    FractionMode fraction_mode = FractionMode::Float;
    bool symbolic_mode = false;
    bool infinite_mode = false;
    bool polar_mode = false;
    int word_size = 32;
    bool twos_complement = false;
    int group_digits = 0;
    char group_char = ',';
    char point_char = '.';
    ComplexFormat complex_format = ComplexFormat::Pair;
    bool leading_zeros = false;

    // Transient flags (cleared after each command)
    bool inverse_flag = false;
    bool hyperbolic_flag = false;
    bool keep_args_flag = false;

    void clear_transient_flags() {
        inverse_flag = false;
        hyperbolic_flag = false;
        keep_args_flag = false;
    }
};

} // namespace sc
