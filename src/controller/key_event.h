#pragma once

#include <string>
#include <cstdint>

namespace sc {

// Represents a single key event from the UI layer.
struct KeyEvent {
    enum Type : uint8_t {
        Char,       // printable character
        Special,    // named special key (e.g. "RET", "DEL", "TAB", "SPC")
        Modified,   // modifier + key (e.g. "M-RET", "C-u")
    };

    Type type;
    char ch = 0;            // for Char type
    std::string name;       // for Special and Modified types

    static KeyEvent character(char c) { return {Char, c, {}}; }
    static KeyEvent special(std::string n) { return {Special, 0, std::move(n)}; }
    static KeyEvent modified(std::string n) { return {Modified, 0, std::move(n)}; }

    // Return a string key suitable for KeyMap lookup
    std::string key_name() const;
};

} // namespace sc
