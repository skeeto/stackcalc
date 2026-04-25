#pragma once

#include "key_event.hpp"
#include "key_map.hpp"
#include "input_state.hpp"
#include "display_state.hpp"
#include "stack.hpp"
#include "variables.hpp"
#include "formatter.hpp"

namespace sc {

class Controller {
public:
    Controller();

    // Process a key event. Returns true if handled.
    bool process_key(const KeyEvent& key);

    // Wipe everything back to a pristine state: empty stack, empty trail,
    // cleared undo history, cleared variables, default modes, no pending
    // input or prefix, no modifier flags. Not undoable.
    void reset();

    // Parse `text` (typically clipboard contents) as a single number
    // and push it onto the stack. Accepts the same syntax as keyboard
    // number entry — integers, decimals ("3.14"), fractions ("1:3"),
    // radix-prefixed ("16#FF"), scientific ("1.5e10") — plus a few
    // friendlier external conventions:
    //   * leading "-" (rather than our "_") for negatives;
    //   * leading "0x"/"0b"/"0o" → "16#"/"2#"/"8#";
    //   * digit-grouping commas and internal spaces are stripped
    //     before parsing ("1,234,567" → "1234567").
    // Multi-line text and unparseable strings are rejected with a
    // descriptive message_; the stack is unchanged. Trail tag "paste".
    void paste_text(const std::string& text);

    // Get the current display state (read-only snapshot).
    DisplayState display() const;

    // Direct access for testing
    Stack& stack() { return stack_; }
    const Stack& stack() const { return stack_; }
    Variables& variables() { return vars_; }
    const Variables& variables() const { return vars_; }
    InputState& input() { return input_; }

private:
    // Execute a command by name.
    void execute(const std::string& command);

    // Execute a variable command (s s/r/t/x/u/+/-/*/)/) once we've read
    // the single-letter variable name.
    void execute_var_command(const std::string& command, const std::string& name);

    // Finalize number entry if active, pushing the value onto the stack.
    void finalize_entry();

    // Build the mode line string.
    std::string build_mode_line() const;

    Stack stack_;
    Variables vars_;
    InputState input_;
    KeyMap keymap_;
    std::string pending_prefix_;
    std::string pending_var_command_;  // command name waiting for a var name keystroke
    std::string message_;
};

} // namespace sc
