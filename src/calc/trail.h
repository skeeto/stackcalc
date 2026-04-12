#pragma once

#include "value.h"
#include <string>
#include <vector>
#include <optional>

namespace sc {

struct TrailEntry {
    std::string tag;   // short tag like "add", "sqrt"
    ValuePtr value;
    size_t index;
};

class Trail {
public:
    void record(const std::string& tag, ValuePtr v);

    size_t size() const { return entries_.size(); }
    bool empty() const { return entries_.empty(); }
    const TrailEntry& at(size_t i) const { return entries_.at(i); }

    // Trail pointer
    size_t pointer() const { return pointer_; }
    void set_pointer(size_t pos);
    void move_pointer(int delta);
    ValuePtr yank() const;
    ValuePtr yank(int offset) const;

private:
    std::vector<TrailEntry> entries_;
    size_t pointer_ = 0;
    size_t next_index_ = 1;
};

} // namespace sc
