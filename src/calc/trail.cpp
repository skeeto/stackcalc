#include "trail.hpp"
#include <algorithm>

namespace sc {

void Trail::record(const std::string& tag, ValuePtr v) {
    entries_.push_back({tag, std::move(v), next_index_++});
    pointer_ = entries_.size() - 1;
}

void Trail::set_pointer(size_t pos) {
    if (!entries_.empty()) {
        pointer_ = std::min(pos, entries_.size() - 1);
    }
}

void Trail::move_pointer(int delta) {
    if (entries_.empty()) return;
    int new_pos = static_cast<int>(pointer_) + delta;
    new_pos = std::max(0, std::min(new_pos, static_cast<int>(entries_.size()) - 1));
    pointer_ = static_cast<size_t>(new_pos);
}

ValuePtr Trail::yank() const {
    if (entries_.empty()) return nullptr;
    return entries_[pointer_].value;
}

ValuePtr Trail::yank(int offset) const {
    if (entries_.empty()) return nullptr;
    int pos = static_cast<int>(pointer_) + offset;
    if (pos < 0 || pos >= static_cast<int>(entries_.size())) return nullptr;
    return entries_[pos].value;
}

void Trail::kill_at_pointer() {
    if (entries_.empty()) return;
    entries_.erase(entries_.begin() + pointer_);
    if (entries_.empty()) {
        pointer_ = 0;
    } else if (pointer_ >= entries_.size()) {
        pointer_ = entries_.size() - 1;
    }
}

} // namespace sc
