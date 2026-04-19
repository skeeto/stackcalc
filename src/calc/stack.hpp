#pragma once

#include "value.hpp"
#include "trail.hpp"
#include "undo.hpp"
#include "modes.hpp"
#include <vector>
#include <stdexcept>

namespace sc {

class StackError : public std::runtime_error {
    using std::runtime_error::runtime_error;
};

class Stack {
public:
    // --- Stack access ---
    size_t size() const { return stack_.size(); }
    bool empty() const { return stack_.empty(); }

    // Top of stack (depth 0 = top, 1 = second, etc.)
    ValuePtr top(size_t depth = 0) const;
    std::vector<ValuePtr> top_n(size_t n) const;

    // --- Mutating operations (all record undo) ---

    void push(ValuePtr v);

    ValuePtr pop();
    std::vector<ValuePtr> pop_n(size_t n);

    // RET/SPC: duplicate top (or element at depth n)
    void dup(size_t depth = 0);

    // DEL: discard top (or element at depth n)
    void drop(size_t depth = 0);

    // TAB: swap top two; with n, rotate top n elements down
    void swap();
    void roll_down(size_t n = 2);

    // M-TAB: rotate top n elements up (default 3)
    void roll_up(size_t n = 3);

    // --- Undo/redo ---
    void begin_command();
    void end_command(const std::string& trail_tag, const std::vector<ValuePtr>& results);
    void discard_command();
    void undo();
    void redo();

    // --- Trail ---
    Trail& trail() { return trail_; }
    const Trail& trail() const { return trail_; }

    // --- Undo manager (used by persistence) ---
    UndoManager& undo_mgr() { return undo_mgr_; }
    const UndoManager& undo_mgr() const { return undo_mgr_; }

    // --- Last args (M-RET) ---
    const std::vector<ValuePtr>& last_args() const { return last_args_; }
    void set_last_args(std::vector<ValuePtr> args) { last_args_ = std::move(args); }

    // --- Modes ---
    CalcState& state() { return state_; }
    const CalcState& state() const { return state_; }

    // Full stack access (bottom=0, top=back)
    const std::vector<ValuePtr>& elements() const { return stack_; }

private:
    void require_depth(size_t n) const;

    std::vector<ValuePtr> stack_; // index 0 = bottom, back() = top
    Trail trail_;
    UndoManager undo_mgr_;
    CalcState state_;
    std::vector<ValuePtr> last_args_;
    bool in_command_ = false;
};

} // namespace sc
