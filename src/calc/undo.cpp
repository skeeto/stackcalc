#include "undo.h"

namespace sc {

void UndoManager::save_state(std::vector<ValuePtr> stack) {
    undo_stack_.push_back({std::move(stack)});
    if (undo_stack_.size() > max_depth_) {
        undo_stack_.erase(undo_stack_.begin());
    }
    redo_stack_.clear();
}

bool UndoManager::can_undo() const {
    return !undo_stack_.empty();
}

bool UndoManager::can_redo() const {
    return !redo_stack_.empty();
}

std::vector<ValuePtr> UndoManager::undo(std::vector<ValuePtr> current_stack) {
    if (undo_stack_.empty()) return current_stack;
    redo_stack_.push_back({std::move(current_stack)});
    auto restored = std::move(undo_stack_.back().stack_snapshot);
    undo_stack_.pop_back();
    return restored;
}

std::vector<ValuePtr> UndoManager::redo(std::vector<ValuePtr> current_stack) {
    if (redo_stack_.empty()) return current_stack;
    undo_stack_.push_back({std::move(current_stack)});
    auto restored = std::move(redo_stack_.back().stack_snapshot);
    redo_stack_.pop_back();
    return restored;
}

std::vector<ValuePtr> UndoManager::cancel_save() {
    if (undo_stack_.empty()) return {};
    auto restored = std::move(undo_stack_.back().stack_snapshot);
    undo_stack_.pop_back();
    return restored;
}

} // namespace sc
