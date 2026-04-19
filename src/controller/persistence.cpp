#include "persistence.h"
#include <cctype>
#include <istream>
#include <ostream>
#include <stdexcept>
#include <string>

namespace sc::persistence {

namespace {

// --- S-expression tokenizing helpers (recursive-descent style) ----------

void skip_ws(std::istream& is) {
    while (true) {
        int c = is.peek();
        if (c == EOF) return;
        if (std::isspace(c)) { is.get(); continue; }
        if (c == ';') {                          // line comment
            while (c != EOF && c != '\n') { is.get(); c = is.peek(); }
            continue;
        }
        return;
    }
}

void expect(std::istream& is, char ch) {
    skip_ws(is);
    if (is.peek() != ch) {
        throw std::runtime_error(std::string("expected '") + ch + "'");
    }
    is.get();
}

// Bare token: anything until whitespace, '(', ')', ';', or EOF.
std::string read_atom(std::istream& is) {
    skip_ws(is);
    std::string s;
    int c = is.peek();
    while (c != EOF && !std::isspace(c) && c != '(' && c != ')' && c != ';') {
        s += static_cast<char>(is.get());
        c = is.peek();
    }
    if (s.empty()) throw std::runtime_error("expected atom");
    return s;
}

// Quoted string with backslash escapes for " \ n t.
std::string read_string(std::istream& is) {
    skip_ws(is);
    if (is.peek() != '"') throw std::runtime_error("expected '\"'");
    is.get();
    std::string s;
    while (true) {
        int c = is.get();
        if (c == EOF) throw std::runtime_error("unterminated string");
        if (c == '"') return s;
        if (c == '\\') {
            int n = is.get();
            switch (n) {
                case '"':  s += '"';  break;
                case '\\': s += '\\'; break;
                case 'n':  s += '\n'; break;
                case 't':  s += '\t'; break;
                default:   throw std::runtime_error("unknown escape");
            }
        } else {
            s += static_cast<char>(c);
        }
    }
}

void write_string(std::ostream& os, const std::string& s) {
    os << '"';
    for (char c : s) {
        switch (c) {
            case '"':  os << "\\\""; break;
            case '\\': os << "\\\\"; break;
            case '\n': os << "\\n";  break;
            case '\t': os << "\\t";  break;
            default:   os << c;
        }
    }
    os << '"';
}

// Skip from the position right after a '(' through the matching ')'.
// Used to ignore unknown top-level forms gracefully.
void skip_form_body(std::istream& is) {
    int depth = 1;
    while (depth > 0) {
        int c = is.get();
        if (c == EOF) throw std::runtime_error("unexpected EOF in form");
        if      (c == '(')  depth++;
        else if (c == ')')  depth--;
        else if (c == '"') {                    // string with escapes
            while (true) {
                int sc = is.get();
                if (sc == EOF) throw std::runtime_error("unterminated string");
                if (sc == '\\') is.get();
                else if (sc == '"') break;
            }
        } else if (c == ';') {                  // comment
            while (c != EOF && c != '\n') c = is.get();
        }
    }
}

// --- Mode key/value codec ------------------------------------------------

const char* angular_str(AngularMode m) {
    return m == AngularMode::Radians ? "radians" : "degrees";
}
AngularMode parse_angular(const std::string& s) {
    return s == "radians" ? AngularMode::Radians : AngularMode::Degrees;
}

const char* format_str(DisplayFormat f) {
    switch (f) {
        case DisplayFormat::Fix: return "fix";
        case DisplayFormat::Sci: return "sci";
        case DisplayFormat::Eng: return "eng";
        default:                 return "normal";
    }
}
DisplayFormat parse_format(const std::string& s) {
    if (s == "fix") return DisplayFormat::Fix;
    if (s == "sci") return DisplayFormat::Sci;
    if (s == "eng") return DisplayFormat::Eng;
    return DisplayFormat::Normal;
}

const char* fraction_str(FractionMode m) {
    return m == FractionMode::Fraction ? "fraction" : "float";
}
FractionMode parse_fraction(const std::string& s) {
    return s == "fraction" ? FractionMode::Fraction : FractionMode::Float;
}

const char* complex_str(ComplexFormat f) {
    switch (f) {
        case ComplexFormat::INotation: return "i";
        case ComplexFormat::JNotation: return "j";
        default:                       return "pair";
    }
}
ComplexFormat parse_complex(const std::string& s) {
    if (s == "i") return ComplexFormat::INotation;
    if (s == "j") return ComplexFormat::JNotation;
    return ComplexFormat::Pair;
}

void write_modes(std::ostream& os, const CalcState& s) {
    os << "(mode precision "       << s.precision       << ")\n";
    os << "(mode display_radix "   << s.display_radix   << ")\n";
    os << "(mode angular_mode "    << angular_str(s.angular_mode) << ")\n";
    os << "(mode display_format "  << format_str(s.display_format) << ")\n";
    os << "(mode display_digits "  << s.display_digits  << ")\n";
    os << "(mode fraction_mode "   << fraction_str(s.fraction_mode) << ")\n";
    os << "(mode symbolic_mode "   << (s.symbolic_mode ? "true" : "false") << ")\n";
    os << "(mode infinite_mode "   << (s.infinite_mode ? "true" : "false") << ")\n";
    os << "(mode polar_mode "      << (s.polar_mode    ? "true" : "false") << ")\n";
    os << "(mode word_size "       << s.word_size       << ")\n";
    os << "(mode twos_complement " << (s.twos_complement ? "true" : "false") << ")\n";
    os << "(mode group_digits "    << s.group_digits    << ")\n";
    os << "(mode complex_format "  << complex_str(s.complex_format) << ")\n";
    os << "(mode leading_zeros "   << (s.leading_zeros ? "true" : "false") << ")\n";
}

void apply_mode(CalcState& s, const std::string& key, const std::string& val) {
    if      (key == "precision")        s.precision = std::stoi(val);
    else if (key == "display_radix")    s.display_radix = std::stoi(val);
    else if (key == "angular_mode")     s.angular_mode = parse_angular(val);
    else if (key == "display_format")   s.display_format = parse_format(val);
    else if (key == "display_digits")   s.display_digits = std::stoi(val);
    else if (key == "fraction_mode")    s.fraction_mode = parse_fraction(val);
    else if (key == "symbolic_mode")    s.symbolic_mode = (val == "true");
    else if (key == "infinite_mode")    s.infinite_mode = (val == "true");
    else if (key == "polar_mode")       s.polar_mode    = (val == "true");
    else if (key == "word_size")        s.word_size = std::stoi(val);
    else if (key == "twos_complement")  s.twos_complement = (val == "true");
    else if (key == "group_digits")     s.group_digits = std::stoi(val);
    else if (key == "complex_format")   s.complex_format = parse_complex(val);
    else if (key == "leading_zeros")    s.leading_zeros = (val == "true");
    // Unknown keys silently ignored — supports forward compatibility.
}

const char* infinity_kind_str(Infinity::Kind k) {
    switch (k) {
        case Infinity::Pos:        return "pos";
        case Infinity::Neg:        return "neg";
        case Infinity::Undirected: return "udir";
        case Infinity::NaN:        return "nan";
    }
    return "nan";
}

Infinity::Kind parse_infinity_kind(const std::string& s) {
    if (s == "pos")  return Infinity::Pos;
    if (s == "neg")  return Infinity::Neg;
    if (s == "udir") return Infinity::Undirected;
    return Infinity::NaN;
}

} // anon namespace

// --- Public API ---------------------------------------------------------

void write_value(std::ostream& os, const ValuePtr& v) {
    switch (v->tag()) {
        case Tag::Integer:
            os << "(int " << v->as_integer().v.get_str(10) << ")";
            break;
        case Tag::Fraction: {
            auto& f = v->as_fraction();
            os << "(frac " << f.num.get_str(10) << " " << f.den.get_str(10) << ")";
            break;
        }
        case Tag::DecimalFloat: {
            auto& d = v->as_float();
            os << "(float " << d.mantissa.get_str(10) << " " << d.exponent << ")";
            break;
        }
        case Tag::HMS: {
            auto& h = v->as_hms();
            os << "(hms " << h.hours << " " << h.minutes << " ";
            write_value(os, h.seconds);
            os << ")";
            break;
        }
        case Tag::RectComplex: {
            auto& c = v->as_rect_complex();
            os << "(rect ";
            write_value(os, c.re);
            os << " ";
            write_value(os, c.im);
            os << ")";
            break;
        }
        case Tag::PolarComplex: {
            auto& c = v->as_polar_complex();
            os << "(polar ";
            write_value(os, c.r);
            os << " ";
            write_value(os, c.theta);
            os << ")";
            break;
        }
        case Tag::DateForm: {
            auto& d = v->as_date();
            os << "(date ";
            write_value(os, d.n);
            os << ")";
            break;
        }
        case Tag::ModuloForm: {
            auto& m = v->as_mod();
            os << "(mod ";
            write_value(os, m.n);
            os << " ";
            write_value(os, m.m);
            os << ")";
            break;
        }
        case Tag::ErrorForm: {
            auto& e = v->as_error();
            os << "(error ";
            write_value(os, e.x);
            os << " ";
            write_value(os, e.sigma);
            os << ")";
            break;
        }
        case Tag::Interval: {
            auto& i = v->as_interval();
            os << "(interval " << static_cast<int>(i.mask) << " ";
            write_value(os, i.lo);
            os << " ";
            write_value(os, i.hi);
            os << ")";
            break;
        }
        case Tag::Vector: {
            auto& vec = v->as_vector();
            os << "(vec";
            for (auto& e : vec.elements) {
                os << " ";
                write_value(os, e);
            }
            os << ")";
            break;
        }
        case Tag::Infinity: {
            os << "(inf " << infinity_kind_str(v->as_infinity().kind) << ")";
            break;
        }
    }
}

ValuePtr read_value(std::istream& is) {
    expect(is, '(');
    std::string tag = read_atom(is);

    if (tag == "int") {
        std::string n = read_atom(is);
        expect(is, ')');
        return Value::make_integer(mpz_class(n));
    }
    if (tag == "frac") {
        std::string n = read_atom(is), d = read_atom(is);
        expect(is, ')');
        return Value::make_fraction(mpz_class(n), mpz_class(d));
    }
    if (tag == "float") {
        std::string m = read_atom(is), e = read_atom(is);
        expect(is, ')');
        return Value::make_float(mpz_class(m), std::stoi(e));
    }
    if (tag == "hms") {
        std::string h = read_atom(is), m = read_atom(is);
        auto s = read_value(is);
        expect(is, ')');
        return Value::make_hms(std::stoi(h), std::stoi(m), s);
    }
    if (tag == "rect") {
        auto re = read_value(is), im = read_value(is);
        expect(is, ')');
        return Value::make_rect_complex(re, im);
    }
    if (tag == "polar") {
        auto r = read_value(is), t = read_value(is);
        expect(is, ')');
        return Value::make_polar_complex(r, t);
    }
    if (tag == "date") {
        auto n = read_value(is);
        expect(is, ')');
        return Value::make_date(n);
    }
    if (tag == "mod") {
        auto n = read_value(is), m = read_value(is);
        expect(is, ')');
        return Value::make_mod(n, m);
    }
    if (tag == "error") {
        auto x = read_value(is), s = read_value(is);
        expect(is, ')');
        return Value::make_error(x, s);
    }
    if (tag == "interval") {
        std::string mask = read_atom(is);
        auto lo = read_value(is), hi = read_value(is);
        expect(is, ')');
        return Value::make_interval(static_cast<uint8_t>(std::stoi(mask)), lo, hi);
    }
    if (tag == "vec") {
        std::vector<ValuePtr> elements;
        skip_ws(is);
        while (is.peek() != ')') {
            elements.push_back(read_value(is));
            skip_ws(is);
        }
        is.get();
        return Value::make_vector(std::move(elements));
    }
    if (tag == "inf") {
        std::string k = read_atom(is);
        expect(is, ')');
        return Value::make_infinity(parse_infinity_kind(k));
    }
    throw std::runtime_error("unknown value tag: " + tag);
}

void save(std::ostream& os, const Controller& ctrl) {
    os << "(stackcalc-state " << SCHEMA_VERSION << ")\n";

    write_modes(os, ctrl.stack().state());

    // Stack: bottom-up order matches our internal vector.
    for (auto& v : ctrl.stack().elements()) {
        os << "(stack ";
        write_value(os, v);
        os << ")\n";
    }

    // Trail: chronological order, plus the pointer.
    auto& trail = ctrl.stack().trail();
    for (size_t i = 0; i < trail.size(); ++i) {
        auto& e = trail.at(i);
        os << "(trail ";
        write_string(os, e.tag);
        os << " ";
        write_value(os, e.value);
        os << ")\n";
    }
    if (!trail.empty()) {
        os << "(trail_pointer " << trail.pointer() << ")\n";
    }

    // Variables (named).
    for (auto& name : ctrl.variables().names()) {
        auto v = ctrl.variables().recall(name);
        if (!v) continue;
        os << "(var ";
        write_string(os, name);
        os << " ";
        write_value(os, *v);
        os << ")\n";
    }

    // Quick registers — only ones with a value.
    for (int i = 0; i < 10; ++i) {
        auto v = ctrl.variables().recall_quick(i);
        if (!v) continue;
        os << "(qreg " << i << " ";
        write_value(os, *v);
        os << ")\n";
    }

    // Undo / redo history. Save oldest-first so loading just pushes back.
    for (auto& frame : ctrl.stack().undo_mgr().undo_stack()) {
        os << "(undo";
        for (auto& v : frame.stack_snapshot) { os << " "; write_value(os, v); }
        os << ")\n";
    }
    for (auto& frame : ctrl.stack().undo_mgr().redo_stack()) {
        os << "(redo";
        for (auto& v : frame.stack_snapshot) { os << " "; write_value(os, v); }
        os << ")\n";
    }
}

bool load(std::istream& is, Controller& ctrl) {
    ctrl.reset();

    try {
        // Header: (stackcalc-state V)
        expect(is, '(');
        std::string head = read_atom(is);
        if (head != "stackcalc-state") return false;
        std::string ver = read_atom(is);
        if (std::stoi(ver) != SCHEMA_VERSION) {
            ctrl.reset();
            return false;
        }
        expect(is, ')');

        // Accumulators for things we apply at the end.
        std::vector<ValuePtr> stack_acc;
        std::vector<UndoFrame> undo_acc;
        std::vector<UndoFrame> redo_acc;
        std::optional<size_t> saved_trail_pointer;

        // Helper to read a snapshot list (for undo/redo).
        auto read_snapshot = [&]() {
            UndoFrame f;
            skip_ws(is);
            while (is.peek() != ')') {
                f.stack_snapshot.push_back(read_value(is));
                skip_ws(is);
            }
            is.get();  // ')'
            return f;
        };

        while (true) {
            skip_ws(is);
            if (is.peek() == EOF) break;
            expect(is, '(');
            std::string form = read_atom(is);

            if (form == "mode") {
                std::string key = read_atom(is);
                std::string val = read_atom(is);
                expect(is, ')');
                apply_mode(ctrl.stack().state(), key, val);
            }
            else if (form == "stack") {
                stack_acc.push_back(read_value(is));
                expect(is, ')');
            }
            else if (form == "trail") {
                std::string tag = read_string(is);
                ValuePtr v = read_value(is);
                expect(is, ')');
                ctrl.stack().trail().record(tag, v);
            }
            else if (form == "trail_pointer") {
                saved_trail_pointer = static_cast<size_t>(std::stoi(read_atom(is)));
                expect(is, ')');
            }
            else if (form == "var") {
                std::string name = read_string(is);
                ValuePtr v = read_value(is);
                expect(is, ')');
                ctrl.variables().store(name, v);
            }
            else if (form == "qreg") {
                int n = std::stoi(read_atom(is));
                ValuePtr v = read_value(is);
                expect(is, ')');
                if (n >= 0 && n < 10) ctrl.variables().store_quick(n, v);
            }
            else if (form == "undo") {
                undo_acc.push_back(read_snapshot());
            }
            else if (form == "redo") {
                redo_acc.push_back(read_snapshot());
            }
            else {
                // Unknown form — skip its body. Lets the GUI append its own
                // (window ...) record without persistence having to know.
                skip_form_body(is);
            }
        }

        // Apply accumulated state.
        for (auto& v : stack_acc) ctrl.stack().push(v);
        if (saved_trail_pointer) {
            ctrl.stack().trail().set_pointer(*saved_trail_pointer);
        }
        ctrl.stack().undo_mgr().load(std::move(undo_acc), std::move(redo_acc));

        return true;
    } catch (...) {
        ctrl.reset();
        return false;
    }
}

} // namespace sc::persistence
