#include "controller.h"
#include "operations.h"
#include "scientific.h"
#include "constants.h"
#include "vector.h"
#include <cctype>
#include <sstream>

namespace sc {

Controller::Controller() {}

bool Controller::process_key(const KeyEvent& key) {
    message_.clear();
    std::string kn = key.key_name();

    // If a variable command (s s, s r, etc.) is waiting for its name, the
    // next keystroke is consumed as a single-letter variable name.
    if (!pending_var_command_.empty()) {
        std::string cmd = pending_var_command_;
        pending_var_command_.clear();
        if (key.type == KeyEvent::Char &&
            std::isalpha(static_cast<unsigned char>(key.ch))) {
            std::string name(1, key.ch);
            try {
                execute_var_command(cmd, name);
            } catch (const std::exception& e) {
                stack_.discard_command();
                message_ = e.what();
            }
        } else {
            message_ = "Cancelled (variable name must be a letter)";
        }
        return true;
    }

    // If we have a pending prefix, look up the two-key sequence
    if (!pending_prefix_.empty()) {
        std::string prefix = pending_prefix_;
        pending_prefix_.clear();
        auto cmd = keymap_.lookup_seq(prefix, kn);
        if (cmd) {
            try {
                execute(*cmd);
            } catch (const std::exception& e) {
                stack_.discard_command();  // roll back any partial mutation
                message_ = e.what();
            }
            return true;
        }
        // Not a valid sequence; fall through
        message_ = "Unknown key sequence: " + prefix + " " + kn;
        return true;
    }

    // When no number entry is in progress, prefer keymap bindings over
    // starting a new entry. Otherwise, in radix > 10, command keys that
    // happen to be hex digits ('d', 'F', 'A', 'B', 'C', 'E', etc.) would
    // be silently swallowed by the number-entry state machine and the
    // user could no longer reach those commands.
    if (key.type == KeyEvent::Char && !input_.active()) {
        if (keymap_.is_prefix(kn)) {
            pending_prefix_ = kn;
            return true;
        }
        if (auto cmd = keymap_.lookup(kn)) {
            try {
                execute(*cmd);
            } catch (const std::exception& e) {
                stack_.discard_command();
                message_ = e.what();
            }
            return true;
        }
    }

    // Try to feed to number entry first (digits, decimal, sign, etc.)
    if (key.type == KeyEvent::Char && input_.feed(key.ch, stack_.state())) {
        return true;
    }

    // Check if this key starts a prefix sequence
    if (keymap_.is_prefix(kn)) {
        pending_prefix_ = kn;
        return true;
    }

    // Look up single-key command
    auto cmd = keymap_.lookup(kn);
    if (!cmd) {
        return false; // unhandled
    }

    try {
        execute(*cmd);
    } catch (const std::exception& e) {
        stack_.discard_command();  // roll back any partial mutation
        message_ = e.what();
    }
    return true;
}

void Controller::finalize_entry() {
    auto v = input_.finalize(stack_.state());
    if (v) {
        stack_.begin_command();
        stack_.push(v);
        stack_.end_command("", {v});
    }
}

void Controller::execute(const std::string& command) {
    auto& state = stack_.state();

    // --- Modifier flags ---
    if (command == "inverse") { state.inverse_flag = !state.inverse_flag; return; }
    if (command == "hyperbolic") { state.hyperbolic_flag = !state.hyperbolic_flag; return; }
    if (command == "keep_args") { state.keep_args_flag = !state.keep_args_flag; return; }

    // --- Stack operations ---
    if (command == "enter") {
        if (input_.active()) {
            finalize_entry();
        } else if (stack_.size() > 0) {
            stack_.dup();
        }
        state.clear_transient_flags();
        return;
    }

    // --- Quick registers (t N to store, r N to recall) ---
    if (command.size() == 8 && command.compare(0, 7, "qstore_") == 0) {
        int n = command[7] - '0';
        if (stack_.size() == 0) { message_ = "Stack underflow"; return; }
        finalize_entry();
        vars_.store_quick(n, stack_.top());     // peek (Emacs convention)
        return;
    }
    if (command.size() == 9 && command.compare(0, 8, "qrecall_") == 0) {
        int n = command[8] - '0';
        auto v = vars_.recall_quick(n);
        if (!v) { message_ = std::string("q") + command[8] + " is empty"; return; }
        finalize_entry();
        stack_.begin_command();
        stack_.push(*v);
        stack_.end_command(std::string("rcl-q") + command[8], {*v});
        return;
    }

    // --- Variable storage / recall (s s, s r, s t, s x, s u, s + - * /) ---
    // Each just defers: the next keystroke is consumed as the variable name.
    if (command == "store"      || command == "recall"    ||
        command == "store_into" || command == "exchange"  ||
        command == "unstore"    ||
        command == "store_add"  || command == "store_sub" ||
        command == "store_mul"  || command == "store_div") {
        finalize_entry();  // commit any pending number entry first
        state.clear_transient_flags();
        pending_var_command_ = command;
        return;
    }

    // --- For all other commands, finalize number entry first ---
    finalize_entry();

    bool inv = state.inverse_flag;
    bool hyp = state.hyperbolic_flag;
    state.clear_transient_flags();
    if (command == "drop") { stack_.drop(); return; }
    if (command == "swap") { stack_.swap(); return; }
    if (command == "roll_up") { stack_.roll_up(3); return; }
    if (command == "last_args") {
        const auto args = stack_.last_args();   // copy: push() may reallocate
        if (!args.empty()) {
            stack_.begin_command();
            for (auto& v : args) stack_.push(v);
            stack_.end_command("args", args);
        }
        return;
    }
    if (command == "undo") { stack_.undo(); return; }
    if (command == "redo") { stack_.redo(); return; }

    // --- Binary arithmetic ---
    if (command == "add") { ops::add(stack_); return; }
    if (command == "sub") { ops::sub(stack_); return; }
    if (command == "mul") { ops::mul(stack_); return; }
    if (command == "div") { ops::div(stack_); return; }
    if (command == "power") { ops::power(stack_); return; }
    if (command == "mod") { ops::mod(stack_); return; }
    if (command == "idiv") { ops::idiv(stack_); return; }

    // --- Unary arithmetic ---
    if (command == "neg") { ops::neg(stack_); return; }
    if (command == "inv") { ops::inv(stack_); return; }
    if (command == "abs") { ops::abs_op(stack_); return; }

    // --- sqrt / square ---
    if (command == "sqrt") {
        if (inv) {
            // I Q = square
            stack_.begin_command();
            auto a = stack_.pop();
            auto r = arith::mul(a, a, state.precision);
            stack_.push(r);
            stack_.end_command("sqr", {r});
        } else {
            ops::sqrt_op(stack_);
        }
        return;
    }

    // --- Rounding ---
    if (command == "floor") {
        if (inv) { ops::ceil_op(stack_); } // I F = ceil
        else { ops::floor_op(stack_); }
        return;
    }
    if (command == "round") {
        if (inv) { ops::trunc_op(stack_); } // I R = trunc
        else { ops::round_op(stack_); }
        return;
    }

    // --- Scientific: sin/cos/tan ---
    if (command == "sin") {
        stack_.begin_command();
        auto a = stack_.pop();
        ValuePtr r;
        if (hyp && inv) r = scientific::arcsinh(a, state.precision);
        else if (hyp) r = scientific::sinh(a, state.precision);
        else if (inv) r = scientific::arcsin(a, state.precision, state.angular_mode);
        else r = scientific::sin(a, state.precision, state.angular_mode);
        stack_.push(r);
        stack_.end_command(hyp ? (inv ? "asinh" : "sinh") : (inv ? "asin" : "sin"), {r});
        return;
    }
    if (command == "cos") {
        stack_.begin_command();
        auto a = stack_.pop();
        ValuePtr r;
        if (hyp && inv) r = scientific::arccosh(a, state.precision);
        else if (hyp) r = scientific::cosh(a, state.precision);
        else if (inv) r = scientific::arccos(a, state.precision, state.angular_mode);
        else r = scientific::cos(a, state.precision, state.angular_mode);
        stack_.push(r);
        stack_.end_command(hyp ? (inv ? "acosh" : "cosh") : (inv ? "acos" : "cos"), {r});
        return;
    }
    if (command == "tan") {
        stack_.begin_command();
        auto a = stack_.pop();
        ValuePtr r;
        if (hyp && inv) r = scientific::arctanh(a, state.precision);
        else if (hyp) r = scientific::tanh(a, state.precision);
        else if (inv) r = scientific::arctan(a, state.precision, state.angular_mode);
        else r = scientific::tan(a, state.precision, state.angular_mode);
        stack_.push(r);
        stack_.end_command(hyp ? (inv ? "atanh" : "tanh") : (inv ? "atan" : "tan"), {r});
        return;
    }

    // --- Logarithmic ---
    if (command == "ln") {
        stack_.begin_command();
        auto a = stack_.pop();
        ValuePtr r;
        if (hyp) {
            // H L = log10, H I L = exp10
            r = inv ? scientific::exp10(a, state.precision) : scientific::log10(a, state.precision);
        } else {
            r = inv ? scientific::exp(a, state.precision) : scientific::ln(a, state.precision);
        }
        stack_.push(r);
        stack_.end_command(hyp ? (inv ? "exp10" : "log10") : (inv ? "exp" : "ln"), {r});
        return;
    }
    if (command == "exp") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = inv ? scientific::ln(a, state.precision) : scientific::exp(a, state.precision);
        stack_.push(r);
        stack_.end_command(inv ? "ln" : "exp", {r});
        return;
    }
    if (command == "logb") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = inv
            ? arith::power(args[1], args[0], state.precision)  // I B = alog: b^a
            : scientific::logb(args[0], args[1], state.precision);
        stack_.push(r);
        stack_.end_command(inv ? "alog" : "logb", {r});
        return;
    }

    // --- Constants ---
    if (command == "pi") {
        ValuePtr r;
        const char* tag;
        if (hyp && inv)   { r = constants::phi(state.precision);   tag = "phi"; }
        else if (hyp)     { r = constants::e(state.precision);     tag = "e"; }
        else if (inv)     { r = constants::gamma(state.precision); tag = "gam"; }
        else              { r = constants::pi(state.precision);    tag = "pi"; }
        stack_.begin_command();
        stack_.push(r);
        stack_.end_command(tag, {r});
        return;
    }

    // --- Combinatorics ---
    if (command == "factorial") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::factorial(a, state.precision);
        stack_.push(r);
        stack_.end_command("!", {r});
        return;
    }
    if (command == "choose") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        ValuePtr r;
        if (hyp) r = scientific::permutation(args[0], args[1], state.precision);
        else r = scientific::choose(args[0], args[1], state.precision);
        stack_.push(r);
        stack_.end_command(hyp ? "perm" : "choose", {r});
        return;
    }
    if (command == "dfact") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::double_factorial(a);
        stack_.push(r);
        stack_.end_command("dfact", {r});
        return;
    }
    if (command == "arctan2") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = scientific::arctan2(args[0], args[1], state.precision, state.angular_mode);
        stack_.push(r);
        stack_.end_command("atan2", {r});
        return;
    }
    if (command == "expm1") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::expm1(a, state.precision);
        stack_.push(r);
        stack_.end_command("expm1", {r});
        return;
    }
    if (command == "lnp1") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::lnp1(a, state.precision);
        stack_.push(r);
        stack_.end_command("lnp1", {r});
        return;
    }
    if (command == "ilog") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = scientific::ilog(args[0], args[1]);   // floor(log_args[1](args[0]))
        stack_.push(r);
        stack_.end_command("ilog", {r});
        return;
    }
    if (command == "gamma") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::gamma_fn(a, state.precision);
        stack_.push(r);
        stack_.end_command("gam", {r});
        return;
    }

    // --- Number theory ---
    if (command == "gcd") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = scientific::gcd(args[0], args[1]);
        stack_.push(r);
        stack_.end_command("gcd", {r});
        return;
    }
    if (command == "lcm") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = scientific::lcm(args[0], args[1]);
        stack_.push(r);
        stack_.end_command("lcm", {r});
        return;
    }
    if (command == "next_prime") {
        stack_.begin_command();
        auto a = stack_.pop();
        ValuePtr r;
        if (inv) r = scientific::prev_prime(a);
        else r = scientific::next_prime(a);
        stack_.push(r);
        stack_.end_command(inv ? "prevp" : "nextp", {r});
        return;
    }
    if (command == "prime_test") {
        if (stack_.size() == 0) { message_ = "Stack underflow"; return; }
        auto a = stack_.elements().back();
        auto r = scientific::prime_test(a);
        int v = static_cast<int>(r->as_integer().v.get_si());
        if (v == 2) message_ = "Definitely prime";
        else if (v == 1) message_ = "Probably prime";
        else message_ = "Not prime";
        return;
    }
    if (command == "prime_factors") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::prime_factors(a);
        stack_.push(r);
        stack_.end_command("factor", {r});
        return;
    }
    if (command == "totient") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::totient(a);
        stack_.push(r);
        stack_.end_command("totient", {r});
        return;
    }
    if (command == "random") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = scientific::random(a);
        stack_.push(r);
        stack_.end_command("random", {r});
        return;
    }
    if (command == "extended_gcd") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = scientific::extended_gcd(args[0], args[1]);
        stack_.push(r);
        stack_.end_command("egcd", {r});
        return;
    }
    if (command == "mod_pow") {
        stack_.begin_command();
        auto args = stack_.pop_n(3);
        auto r = scientific::mod_pow(args[0], args[1], args[2]);
        stack_.push(r);
        stack_.end_command("modpow", {r});
        return;
    }

    // --- Display format ---
    if (command == "display_normal") { state.display_format = DisplayFormat::Normal; return; }
    if (command == "display_fix")    { state.display_format = DisplayFormat::Fix; return; }
    if (command == "display_sci")    { state.display_format = DisplayFormat::Sci; return; }
    if (command == "display_eng")    { state.display_format = DisplayFormat::Eng; return; }
    if (command == "radix_2")        { state.display_radix = 2; return; }
    if (command == "radix_8")        { state.display_radix = 8; return; }
    if (command == "radix_16")       { state.display_radix = 16; return; }
    if (command == "radix_10")       { state.display_radix = 10; return; }
    if (command == "radix_n") {
        // Pop the top value as the new display radix (must be 2..36).
        // Snapshot so undo restores the popped value.
        if (stack_.size() > 0) {
            stack_.begin_command();
            auto v = stack_.pop();
            if (v->is_integer()) {
                int r = static_cast<int>(v->as_integer().v.get_si());
                if (r >= 2 && r <= 36) {
                    state.display_radix = r;
                    stack_.end_command("rdx", {});
                } else {
                    stack_.discard_command();
                    message_ = "Radix must be 2-36";
                }
            } else {
                stack_.discard_command();
                message_ = "Radix requires an integer";
            }
        }
        return;
    }
    if (command == "leading_zeros")  { state.leading_zeros = !state.leading_zeros; return; }
    if (command == "grouping")       { state.group_digits = state.group_digits > 0 ? 0 : 3; return; }
    if (command == "complex_pair")   { state.complex_format = ComplexFormat::Pair; return; }
    if (command == "complex_i")      { state.complex_format = ComplexFormat::INotation; return; }
    if (command == "complex_j")      { state.complex_format = ComplexFormat::JNotation; return; }

    // --- Angular mode ---
    if (command == "mode_deg") { state.angular_mode = AngularMode::Degrees; return; }
    if (command == "mode_rad") { state.angular_mode = AngularMode::Radians; return; }
    if (command == "mode_frac") {
        state.fraction_mode = (state.fraction_mode == FractionMode::Fraction)
            ? FractionMode::Float : FractionMode::Fraction;
        return;
    }
    if (command == "mode_symbolic") { state.symbolic_mode = !state.symbolic_mode; return; }
    if (command == "mode_infinite") { state.infinite_mode = !state.infinite_mode; return; }

    // --- Precision ---
    if (command == "precision") {
        // Pop the top value as the new precision. Snapshot so undo restores
        // the popped value. (The mode change itself isn't reversible by undo
        // — only the stack effect is.)
        if (stack_.size() > 0) {
            stack_.begin_command();
            auto v = stack_.pop();
            if (v->is_integer()) {
                int p = static_cast<int>(v->as_integer().v.get_si());
                if (p >= 1 && p <= 1000) {
                    state.precision = p;
                    stack_.end_command("prec", {});
                } else {
                    stack_.discard_command();
                    message_ = "Precision must be 1-1000";
                }
            } else {
                stack_.discard_command();
                message_ = "Precision requires an integer";
            }
        }
        return;
    }

    // --- Vector ops ---
    if (command == "transpose") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = vector_ops::transpose(a->as_vector());
        stack_.push(r);
        stack_.end_command("trn", {r});
        return;
    }
    if (command == "vlength") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = Value::make_integer(vector_ops::length(a->as_vector()));
        stack_.push(r);
        stack_.end_command("vlen", {r});
        return;
    }
    if (command == "vreverse") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = vector_ops::reverse(a->as_vector());
        stack_.push(r);
        stack_.end_command("rev", {r});
        return;
    }
    if (command == "vhead") {
        stack_.begin_command();
        auto a = stack_.pop();
        if (inv) {
            auto r = vector_ops::tail(a->as_vector());
            stack_.push(r);
            stack_.end_command("tail", {r});
        } else {
            auto r = vector_ops::head(a->as_vector());
            stack_.push(r);
            stack_.end_command("head", {r});
        }
        return;
    }
    if (command == "vcons") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = vector_ops::cons(args[0], args[1]->as_vector());
        stack_.push(r);
        stack_.end_command("cons", {r});
        return;
    }
    if (command == "determinant") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = vector_ops::determinant(a->as_vector(), state.precision);
        stack_.push(r);
        stack_.end_command("det", {r});
        return;
    }
    if (command == "trace") {
        stack_.begin_command();
        auto a = stack_.pop();
        auto r = vector_ops::trace(a->as_vector(), state.precision);
        stack_.push(r);
        stack_.end_command("tr", {r});
        return;
    }
    if (command == "cross") {
        stack_.begin_command();
        auto args = stack_.pop_n(2);
        auto r = vector_ops::cross(args[0]->as_vector(), args[1]->as_vector(), state.precision);
        stack_.push(r);
        stack_.end_command("cross", {r});
        return;
    }

    message_ = "Unknown command: " + command;
}

DisplayState Controller::display() const {
    DisplayState ds;
    Formatter fmt(stack_.state());

    // Stack entries
    ds.stack_depth = static_cast<int>(stack_.size());
    for (auto& v : stack_.elements()) {
        ds.stack_entries.push_back(fmt.format(v));
    }

    // Entry text
    ds.entry_text = input_.text();

    // Mode line
    ds.mode_line = build_mode_line();

    // Trail
    for (int i = 0; i < stack_.trail().size(); ++i) {
        auto& entry = stack_.trail().at(i);
        // Empty tag (e.g. plain number entry) renders as just the value.
        ds.trail_entries.push_back(entry.tag.empty()
            ? fmt.format(entry.value)
            : entry.tag + ": " + fmt.format(entry.value));
    }

    // Message
    ds.message = message_;

    // Flags
    ds.inverse_flag = stack_.state().inverse_flag;
    ds.hyperbolic_flag = stack_.state().hyperbolic_flag;
    ds.keep_args_flag = stack_.state().keep_args_flag;

    // Pending prefix key (or pending variable-command "waiting for name")
    if (!pending_prefix_.empty()) {
        ds.pending_prefix = pending_prefix_;
    } else if (!pending_var_command_.empty()) {
        ds.pending_prefix = pending_var_command_ + " ?";
    }

    return ds;
}

void Controller::execute_var_command(const std::string& command,
                                     const std::string& name) {
    auto& state = stack_.state();

    // Recall: pushes; doesn't need stack input.
    if (command == "recall") {
        auto v = vars_.recall(name);
        if (!v) throw std::invalid_argument("variable '" + name + "' not stored");
        stack_.begin_command();
        stack_.push(*v);
        stack_.end_command("rcl-" + name, {*v});
        return;
    }

    // Unstore: doesn't touch the stack.
    if (command == "unstore") {
        if (!vars_.exists(name))
            throw std::invalid_argument("variable '" + name + "' not stored");
        vars_.unstore(name);
        return;
    }

    // The remaining commands all need a value at top of stack.
    if (stack_.size() == 0) throw StackError("stack underflow");

    // Store: peek top, save under name. Stack unchanged.
    if (command == "store") {
        vars_.store(name, stack_.top());
        return;
    }

    // Store-into: pop top, save under name.
    if (command == "store_into") {
        stack_.begin_command();
        auto v = stack_.pop();
        vars_.store(name, v);
        stack_.end_command("sto-" + name, {});
        return;
    }

    // Exchange: swap variable's value with top of stack.
    if (command == "exchange") {
        stack_.begin_command();
        auto top = stack_.pop();
        auto old = vars_.exchange(name, top);  // throws if var not stored
        stack_.push(old);
        stack_.end_command("xch-" + name, {old});
        return;
    }

    // Arithmetic store: peek top, var := var op top. Stack unchanged.
    if (command == "store_add") { vars_.store_add(name, stack_.top(), state.precision); return; }
    if (command == "store_sub") { vars_.store_sub(name, stack_.top(), state.precision); return; }
    if (command == "store_mul") { vars_.store_mul(name, stack_.top(), state.precision); return; }
    if (command == "store_div") { vars_.store_div(name, stack_.top(), state.precision); return; }

    throw std::invalid_argument("unknown variable command: " + command);
}

std::string Controller::build_mode_line() const {
    auto& s = stack_.state();
    std::ostringstream oss;
    oss << s.precision;
    switch (s.angular_mode) {
        case AngularMode::Degrees: oss << " Deg"; break;
        case AngularMode::Radians: oss << " Rad"; break;
    }
    if (s.fraction_mode == FractionMode::Fraction) oss << " Frac";
    if (s.display_radix != 10) oss << " Radix" << s.display_radix;
    switch (s.display_format) {
        case DisplayFormat::Fix: oss << " Fix"; break;
        case DisplayFormat::Sci: oss << " Sci"; break;
        case DisplayFormat::Eng: oss << " Eng"; break;
        default: break;
    }
    return oss.str();
}

} // namespace sc
