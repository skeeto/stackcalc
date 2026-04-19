#include "stack.h"

namespace sc {

void Stack::require_depth(size_t n) const {
    if (stack_.size() < n) {
        throw StackError("not enough elements on stack");
    }
}

ValuePtr Stack::top(size_t depth) const {
    require_depth(depth + 1);
    return stack_[stack_.size() - 1 - depth];
}

std::vector<ValuePtr> Stack::top_n(size_t n) const {
    require_depth(n);
    return {stack_.end() - n, stack_.end()};
}

void Stack::push(ValuePtr v) {
    stack_.push_back(std::move(v));
}

ValuePtr Stack::pop() {
    require_depth(1);
    auto v = std::move(stack_.back());
    stack_.pop_back();
    return v;
}

std::vector<ValuePtr> Stack::pop_n(size_t n) {
    require_depth(n);
    std::vector<ValuePtr> result(stack_.end() - n, stack_.end());
    stack_.erase(stack_.end() - n, stack_.end());
    return result;
}

void Stack::dup(size_t depth) {
    begin_command();
    auto v = top(depth);
    stack_.push_back(v);
    end_command("dup", {v});
}

void Stack::drop(size_t depth) {
    begin_command();
    require_depth(depth + 1);
    size_t idx = stack_.size() - 1 - depth;
    stack_.erase(stack_.begin() + idx);
    end_command("drop", {});
}

void Stack::swap() {
    begin_command();
    require_depth(2);
    size_t n = stack_.size();
    std::swap(stack_[n - 1], stack_[n - 2]);
    end_command("swap", {});
}

void Stack::roll_down(size_t n) {
    begin_command();
    require_depth(n);
    if (n < 2) { discard_command(); return; }
    // Move top element to position (size - n)
    auto top_val = stack_.back();
    for (size_t i = stack_.size() - 1; i > stack_.size() - n; --i) {
        stack_[i] = stack_[i - 1];
    }
    stack_[stack_.size() - n] = top_val;
    end_command("rldn", {});
}

void Stack::roll_up(size_t n) {
    begin_command();
    require_depth(n);
    if (n < 2) { discard_command(); return; }
    // Move bottom-of-group element to top
    size_t bottom_idx = stack_.size() - n;
    auto bottom_val = stack_[bottom_idx];
    for (size_t i = bottom_idx; i < stack_.size() - 1; ++i) {
        stack_[i] = stack_[i + 1];
    }
    stack_.back() = bottom_val;
    end_command("rlup", {});
}

void Stack::begin_command() {
    if (!in_command_) {
        undo_mgr_.save_state(stack_);
        in_command_ = true;
    }
}

void Stack::end_command(const std::string& trail_tag, const std::vector<ValuePtr>& results) {
    for (auto& v : results) {
        trail_.record(trail_tag, v);
    }
    in_command_ = false;
    state_.clear_transient_flags();
}

void Stack::discard_command() {
    if (in_command_) {
        // Restore from the snapshot saved in begin_command and remove that
        // snapshot from undo history (a failed command shouldn't show up
        // as a no-op step in the undo chain). Invariant: begin_command
        // always called save_state, so cancel_save returns that snapshot.
        stack_ = undo_mgr_.cancel_save();
        in_command_ = false;
    }
}

void Stack::undo() {
    stack_ = undo_mgr_.undo(std::move(stack_));
}

void Stack::redo() {
    stack_ = undo_mgr_.redo(std::move(stack_));
}

} // namespace sc
