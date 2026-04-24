#pragma once

#include "value.hpp"
#include "modes.hpp"
#include <string>
#include <vector>

namespace sc {

// One trail row in the display snapshot: an operation tag (e.g.
// "add", "neg", possibly empty for plain number entry) and the
// formatted result value. Distinct from sc::TrailEntry in
// src/calc/trail.hpp, which holds the unformatted ValuePtr.
struct TrailRow {
    std::string tag;
    std::string value;
    bool operator==(const TrailRow&) const = default;
};

// Read-only snapshot of the calculator's display state.
// A GUI/TUI reads this to render.
struct DisplayState {
    // Stack entries, formatted as strings (index 0 = bottom, back = top)
    std::vector<std::string> stack_entries;

    // The current number-entry text (empty if not entering)
    std::string entry_text;

    // Mode line: e.g. "12 Deg"
    std::string mode_line;

    // Trail entries (split into tag + formatted value).
    std::vector<TrailRow> trail_entries;

    // Index into trail_entries marked by the pointer (for yank/navigation),
    // or -1 if the trail is empty.
    int trail_pointer = -1;

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

    // True iff number entry (input state machine) is currently active.
    // The GUI's cursor-blink timer only runs while this is true.
    bool input_active = false;
};

} // namespace sc
