#pragma once

#include "controller.h"
#include <iosfwd>

// Stream-based serialization for the calculator's full state.
//
// Format: hand-rolled S-expressions, one logical record per line, comments
// after `;` to end of line. Tags use bare atoms; strings are double-quoted.
//
// Top-level grammar:
//   (stackcalc-state V)            -- mandatory header, V = SCHEMA_VERSION
//   (mode KEY VALUE)               -- one mode setting per line
//   (stack VALUE)                  -- one entry per line, in stack-bottom→top order
//   (trail "tag" VALUE)            -- in chronological order
//   (trail_pointer N)
//   (var "name" VALUE)
//   (qreg N VALUE)                 -- N is 0..9
//   (undo VALUE...)                -- one snapshot per line, oldest first
//   (redo VALUE...)
//
// Value forms:
//   (int N) (frac N D) (float MANT EXP) (rect V V) (polar V V)
//   (hms H M VALUE) (date V) (mod V V) (error V V) (interval MASK V V)
//   (vec V V V ...) (inf KIND)        ; KIND ∈ pos|neg|udir|nan
//
// Unknown forms are silently skipped on load (so the GUI can append its
// own (window ...) record without persistence having to know about it).

namespace sc::persistence {

constexpr int SCHEMA_VERSION = 1;

// Write a single value form (no trailing newline).
void write_value(std::ostream& os, const ValuePtr& v);

// Read a single value form, throws std::runtime_error on malformed input.
ValuePtr read_value(std::istream& is);

// Save the entire controller state.
void save(std::ostream& os, const Controller& ctrl);

// Restore controller state from the stream. Returns true on success.
// On failure (malformed file, version mismatch, etc.) the controller is
// reset to a pristine state and false is returned.
bool load(std::istream& is, Controller& ctrl);

} // namespace sc::persistence
