#pragma once

#include "value.h"
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
    void set_max_depth(size_t n) { max_depth_ = n; }

private:
    std::vector<UndoFrame> undo_stack_;
    std::vector<UndoFrame> redo_stack_;
    size_t max_depth_ = 100;
};

} // namespace sc
