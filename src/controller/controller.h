#pragma once

#include "key_event.h"
#include "key_map.h"
#include "input_state.h"
#include "display_state.h"
#include "stack.h"
#include "variables.h"
#include "formatter.h"

namespace sc {

class Controller {
public:
    Controller();

    // Process a key event. Returns true if handled.
    bool process_key(const KeyEvent& key);

    // Get the current display state (read-only snapshot).
    DisplayState display() const;

    // Direct access for testing
    Stack& stack() { return stack_; }
    const Stack& stack() const { return stack_; }
    Variables& variables() { return vars_; }
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
