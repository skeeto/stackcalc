#include "key_map.h"

namespace sc {

KeyMap::KeyMap() {
    setup_defaults();
}

std::optional<std::string> KeyMap::lookup(const std::string& key) const {
    auto it = simple_.find(key);
    if (it != simple_.end()) return it->second;
    return std::nullopt;
}

bool KeyMap::is_prefix(const std::string& key) const {
    return prefix_.find(key) != prefix_.end();
}

std::optional<std::string> KeyMap::lookup_seq(const std::string& prefix, const std::string& key) const {
    auto pit = prefix_.find(prefix);
    if (pit == prefix_.end()) return std::nullopt;
    auto kit = pit->second.find(key);
    if (kit == pit->second.end()) return std::nullopt;
    return kit->second;
}

void KeyMap::bind(const std::string& key, const std::string& command) {
    simple_[key] = command;
}

void KeyMap::bind_seq(const std::string& prefix, const std::string& key, const std::string& command) {
    prefix_[prefix][key] = command;
}

void KeyMap::setup_defaults() {
    // --- Arithmetic ---
    bind("+", "add");
    bind("-", "sub");
    bind("*", "mul");
    bind("/", "div");
    bind("^", "power");
    bind("%", "mod");
    bind("\\", "idiv");
    bind("&", "inv");
    bind("n", "neg");
    bind("Q", "sqrt");
    bind("A", "abs");

    // --- Stack ---
    bind("RET", "enter");
    bind("SPC", "enter");
    bind("DEL", "drop");
    bind("TAB", "swap");
    bind("M-TAB", "roll_up");
    bind("M-RET", "last_args");

    // --- Undo ---
    bind("U", "undo");
    bind("D", "redo");

    // --- Rounding ---
    bind("F", "floor");
    bind("R", "round");

    // --- Modifier flags ---
    bind("I", "inverse");
    bind("H", "hyperbolic");
    bind("K", "keep_args");

    // --- Scientific (S=sin, C=cos, T=tan) ---
    bind("S", "sin");
    bind("C", "cos");
    bind("T", "tan");
    bind("L", "ln");
    bind("E", "exp");
    bind("B", "logb");
    bind("!", "factorial");
    bind("P", "pi");

    // --- Display format (d prefix) ---
    bind_seq("d", "n", "display_normal");
    bind_seq("d", "f", "display_fix");
    bind_seq("d", "s", "display_sci");
    bind_seq("d", "e", "display_eng");
    bind_seq("d", "2", "radix_2");
    bind_seq("d", "8", "radix_8");
    bind_seq("d", "6", "radix_16");
    bind_seq("d", "0", "radix_10");
    bind_seq("d", "r", "radix_n");
    bind_seq("d", "g", "grouping");
    bind_seq("d", "c", "complex_pair");
    bind_seq("d", "i", "complex_i");
    bind_seq("d", "j", "complex_j");
    bind_seq("d", "z", "leading_zeros");

    // --- Mode (m prefix) ---
    bind_seq("m", "d", "mode_deg");
    bind_seq("m", "r", "mode_rad");
    bind_seq("m", "f", "mode_frac");
    bind_seq("m", "s", "mode_symbolic");
    bind_seq("m", "i", "mode_infinite");

    // --- Store/recall (s prefix) ---
    bind_seq("s", "s", "store");
    bind_seq("s", "r", "recall");
    bind_seq("s", "t", "store_into");
    bind_seq("s", "x", "exchange");
    bind_seq("s", "u", "unstore");
    bind_seq("s", "+", "store_add");
    bind_seq("s", "-", "store_sub");
    bind_seq("s", "*", "store_mul");
    bind_seq("s", "/", "store_div");

    // --- Precision ---
    bind("p", "precision");

    // --- Quick registers (Emacs t N store, r N recall) ---
    for (char d = '0'; d <= '9'; ++d) {
        bind_seq("t", std::string(1, d), std::string("qstore_") + d);
        bind_seq("r", std::string(1, d), std::string("qrecall_") + d);
    }

    // --- Number theory / combinatorics (k prefix) ---
    bind_seq("k", "c", "choose");
    bind_seq("k", "d", "dfact");
    bind_seq("k", "g", "gcd");
    bind_seq("k", "l", "lcm");
    bind_seq("k", "r", "random");
    bind_seq("k", "n", "next_prime");
    bind_seq("k", "p", "prime_test");
    bind_seq("k", "f", "prime_factors");
    bind_seq("k", "t", "totient");
    bind_seq("k", "e", "extended_gcd");
    bind_seq("k", "m", "mod_pow");

    // --- Vector (v prefix) ---
    bind_seq("v", "t", "transpose");
    bind_seq("v", "l", "vlength");
    bind_seq("v", "v", "vreverse");
    bind_seq("v", "h", "vhead");
    bind_seq("v", "k", "vcons");

    // --- Scientific (f prefix) ---
    bind_seq("f", "T", "arctan2");
    bind_seq("f", "E", "expm1");
    bind_seq("f", "L", "lnp1");
    bind_seq("f", "I", "ilog");
    bind_seq("f", "g", "gamma");

    // --- Vector reduce (V prefix) ---
    bind_seq("V", "D", "determinant");
    bind_seq("V", "T", "trace");
    bind_seq("V", "C", "cross");
}

} // namespace sc
