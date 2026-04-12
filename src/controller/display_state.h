#pragma once

#include "value.h"
#include "modes.h"
#include <string>
#include <vector>

namespace sc {

// Read-only snapshot of the calculator's display state.
// A GUI/TUI reads this to render.
struct DisplayState {
    // Stack entries, formatted as strings (index 0 = bottom, back = top)
    std::vector<std::string> stack_entries;

    // The current number-entry text (empty if not entering)
    std::string entry_text;

    // Mode line: e.g. "12 Deg"
    std::string mode_line;

    // Trail entries (formatted)
    std::vector<std::string> trail_entries;

    // Message (error, result info, etc.)
    std::string message;

    // Modifier flags currently active
    bool inverse_flag = false;
    bool hyperbolic_flag = false;
    bool keep_args_flag = false;

    // Pending prefix key (e.g. "d", "m", "s") for UI feedback
    std::string pending_prefix;

    // Stack depth
    int stack_depth = 0;
};

} // namespace sc
