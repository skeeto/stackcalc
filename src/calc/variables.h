#pragma once

#include "value.h"
#include <string>
#include <unordered_map>
#include <optional>

namespace sc {

class Variables {
public:
    // Store a named variable
    void store(const std::string& name, ValuePtr value);

    // Recall a named variable (returns nullopt if not stored)
    std::optional<ValuePtr> recall(const std::string& name) const;

    // Unstore (remove) a variable
    void unstore(const std::string& name);

    // Quick registers q0-q9
    void store_quick(int n, ValuePtr value);
    std::optional<ValuePtr> recall_quick(int n) const;

    // Store with arithmetic: s+, s-, s*, s/
    void store_add(const std::string& name, ValuePtr value, int precision);
    void store_sub(const std::string& name, ValuePtr value, int precision);
    void store_mul(const std::string& name, ValuePtr value, int precision);
    void store_div(const std::string& name, ValuePtr value, int precision);

    // Exchange: swap variable value with given value
    ValuePtr exchange(const std::string& name, ValuePtr value);

    // Check if a variable exists
    bool exists(const std::string& name) const;

    // Get all variable names
    std::vector<std::string> names() const;

private:
    std::unordered_map<std::string, ValuePtr> vars_;
    ValuePtr quick_[10] = {};
};

} // namespace sc
