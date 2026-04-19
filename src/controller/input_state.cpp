#include "input_state.h"
#include "decimal_float.h"
#include <stdexcept>

namespace sc {

bool InputState::feed(char ch, const CalcState& /*state*/) {
    // Number entry is ALWAYS decimal regardless of the display radix —
    // typing "10" always means ten, even when display_radix == 16. To
    // enter a non-decimal literal, use the explicit "R#NNN" form (e.g.
    // "16#FF"). This way command keys that double as hex digits ('d',
    // 'f', 'A', 'B', etc.) keep working in hex display mode.

    // If we have a radix prefix (e.g. "16#"), accept digits in that radix.
    auto hash_pos = text_.find('#');
    if (active_ && hash_pos != std::string::npos) {
        int custom_radix = std::stoi(text_.substr(0, hash_pos));
        bool is_custom_digit = false;
        if (custom_radix <= 10) {
            is_custom_digit = (ch >= '0' && ch < '0' + custom_radix);
        } else {
            is_custom_digit = (ch >= '0' && ch <= '9') ||
                              (ch >= 'a' && ch < 'a' + (custom_radix - 10)) ||
                              (ch >= 'A' && ch < 'A' + (custom_radix - 10));
        }
        if (is_custom_digit) {
            text_ += ch;
            return true;
        }
    }

    // Plain decimal digits.
    if (ch >= '0' && ch <= '9') {
        active_ = true;
        text_ += ch;
        return true;
    }

    if (ch == '.') {
        if (!active_) {
            active_ = true;
            text_ = "0.";
        } else if (text_.find('.') == std::string::npos) {
            text_ += '.';
        }
        return true;
    }

    if ((ch == 'e' || ch == 'E') && active_) {
        // Start exponent
        if (text_.find('e') == std::string::npos && text_.find('E') == std::string::npos) {
            text_ += 'e';
            return true;
        }
    }

    // '-' or '+' immediately after the exponent marker is unambiguous (we're
    // mid-number-entry, can't be the binary subtract/add operator), so let
    // them join the entry. This makes typing "1e-3" Just Work; without it,
    // the '-' would dispatch as subtract and finalize an incomplete "1e".
    if ((ch == '-' || ch == '+') && active_ &&
        !text_.empty() && (text_.back() == 'e' || text_.back() == 'E')) {
        text_ += ch;
        return true;
    }

    if (ch == '_') {
        // Negative sign in number entry
        if (!active_) {
            active_ = true;
            text_ = "-";
        } else if (text_.back() == 'e' || text_.back() == 'E') {
            text_ += '-';
        }
        return true;
    }

    if (ch == ':' && active_) {
        // Fraction separator
        if (text_.find(':') == std::string::npos && text_.find('.') == std::string::npos) {
            text_ += ':';
            return true;
        }
    }

    if (ch == '@' && active_) {
        // HMS degree separator
        text_ += '@';
        return true;
    }

    if (ch == '\'' && active_) {
        // HMS minute separator
        text_ += '\'';
        return true;
    }

    if (ch == '"' && active_) {
        // HMS second separator
        text_ += '"';
        return true;
    }

    if (ch == '#' && active_) {
        // Radix prefix: e.g. "16#" to enter hex
        text_ += '#';
        return true;
    }

    // Backspace
    if (ch == '\b' || ch == 127) {
        if (active_ && !text_.empty()) {
            text_.pop_back();
            if (text_.empty() || text_ == "-") {
                cancel();
            }
            return true;
        }
    }

    return false;
}

ValuePtr InputState::finalize(const CalcState& state) {
    if (!active_) return nullptr;
    auto result = parse(state);
    cancel();
    return result;
}

void InputState::cancel() {
    active_ = false;
    text_.clear();
}

ValuePtr InputState::parse(const CalcState& state) const {
    if (text_.empty() || text_ == "-") return nullptr;

    // Check for radix# prefix
    auto hash_pos = text_.find('#');
    if (hash_pos != std::string::npos) {
        int radix = std::stoi(text_.substr(0, hash_pos));
        std::string digits = text_.substr(hash_pos + 1);
        if (digits.empty()) return nullptr;
        mpz_class v;
        v.set_str(digits, radix);
        return Value::make_integer(std::move(v));
    }

    // HMS: contains @
    if (text_.find('@') != std::string::npos) {
        // Parse hours@minutes'seconds"
        int h = 0, m = 0;
        std::string sec_str = "0";
        auto at_pos = text_.find('@');
        h = std::stoi(text_.substr(0, at_pos));
        std::string rest = text_.substr(at_pos + 1);
        auto apos_pos = rest.find('\'');
        if (apos_pos != std::string::npos) {
            if (apos_pos > 0) m = std::stoi(rest.substr(0, apos_pos));
            std::string sec_rest = rest.substr(apos_pos + 1);
            auto quote_pos = sec_rest.find('"');
            if (quote_pos != std::string::npos) sec_str = sec_rest.substr(0, quote_pos);
            else if (!sec_rest.empty()) sec_str = sec_rest;
        } else {
            if (!rest.empty()) m = std::stoi(rest);
        }
        ValuePtr secs;
        if (sec_str.find('.') != std::string::npos) {
            // Parse as float
            auto dot = sec_str.find('.');
            std::string int_part = sec_str.substr(0, dot);
            std::string frac_part = sec_str.substr(dot + 1);
            std::string all = int_part + frac_part;
            int exp = -static_cast<int>(frac_part.size());
            secs = Value::make_float_normalized(mpz_class(all), exp, state.precision);
        } else {
            secs = Value::make_integer(mpz_class(sec_str));
        }
        return Value::make_hms(h, m, secs);
    }

    // Fraction: contains ':'
    if (text_.find(':') != std::string::npos) {
        auto colon = text_.find(':');
        mpz_class num(text_.substr(0, colon));
        mpz_class den(text_.substr(colon + 1));
        return Value::make_fraction(std::move(num), std::move(den));
    }

    // Float: contains '.' or 'e'/'E'
    if (text_.find('.') != std::string::npos || text_.find('e') != std::string::npos) {
        // Parse mantissa and exponent
        std::string s = text_;
        int explicit_exp = 0;
        auto e_pos = s.find('e');
        if (e_pos == std::string::npos) e_pos = s.find('E');
        if (e_pos != std::string::npos) {
            explicit_exp = std::stoi(s.substr(e_pos + 1));
            s = s.substr(0, e_pos);
        }

        auto dot_pos = s.find('.');
        if (dot_pos != std::string::npos) {
            std::string int_part = s.substr(0, dot_pos);
            std::string frac_part = s.substr(dot_pos + 1);
            std::string all_digits = int_part + frac_part;
            int exp = explicit_exp - static_cast<int>(frac_part.size());
            if (all_digits.empty() || all_digits == "-") return Value::zero();
            return Value::make_float_normalized(mpz_class(all_digits), exp, state.precision);
        } else {
            return Value::make_float_normalized(mpz_class(s), explicit_exp, state.precision);
        }
    }

    // Plain integer (always decimal — see the comment in feed()).
    mpz_class v;
    v.set_str(text_, 10);
    return Value::make_integer(std::move(v));
}

} // namespace sc
