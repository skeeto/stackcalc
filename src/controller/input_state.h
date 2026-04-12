#pragma once

#include "value.h"
#include "modes.h"
#include <string>

namespace sc {

// Number-entry state machine.
// Accepts digit/decimal/sign/exponent keys and builds a value.
class InputState {
public:
    // Feed a character. Returns true if consumed (part of number entry).
    bool feed(char ch, const CalcState& state);

    // Finalize: parse the current entry into a Value. Returns nullptr if empty.
    ValuePtr finalize(const CalcState& state);

    // Cancel entry.
    void cancel();

    // Is the user currently entering a number?
    bool active() const { return active_; }

    // The display text of the current entry.
    const std::string& text() const { return text_; }

private:
    bool active_ = false;
    std::string text_;

    // Parse the accumulated text into a value
    ValuePtr parse(const CalcState& state) const;
};

} // namespace sc
