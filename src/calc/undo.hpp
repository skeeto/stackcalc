#pragma once

#include "value.hpp"
#include <vector>
#include <optional>

namespace sc {

// Snapshot of the full stack state for undo/redo
struct UndoFrame {
    std::vector<ValuePtr> stack_snapshot;
};

class UndoManager {
public:
    void save_state(std::vector<ValuePtr> stack);
    bool can_undo() const;
    bool can_redo() const;
    std::vector<ValuePtr> undo(std::vector<ValuePtr> current_stack);
    std::vector<ValuePtr> redo(std::vector<ValuePtr> current_stack);

    // Aborting a command: returns the most recent saved snapshot and removes
    // it from the undo stack, so a failed command leaves no trace in history.
    // Returns an empty vector if nothing is saved.
    std::vector<ValuePtr> cancel_save();

    void set_max_depth(size_t n) { max_depth_ = n; }

    // Accessors used by persistence (read full history for save; load-back
    // bypasses the normal save_state path which would clear redo and cap
    // max-depth).
    const std::vector<UndoFrame>& undo_stack() const { return undo_stack_; }
    const std::vector<UndoFrame>& redo_stack() const { return redo_stack_; }
    void load(std::vector<UndoFrame> u, std::vector<UndoFrame> r) {
        undo_stack_ = std::move(u);
        redo_stack_ = std::move(r);
    }

private:
    std::vector<UndoFrame> undo_stack_;
    std::vector<UndoFrame> redo_stack_;
    size_t max_depth_ = 100;
};

} // namespace sc
