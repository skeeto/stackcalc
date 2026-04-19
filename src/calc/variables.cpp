#include "variables.hpp"
#include "arithmetic.hpp"
#include <stdexcept>

namespace sc {

void Variables::store(const std::string& name, ValuePtr value) {
    vars_[name] = std::move(value);
}

std::optional<ValuePtr> Variables::recall(const std::string& name) const {
    auto it = vars_.find(name);
    if (it == vars_.end()) return std::nullopt;
    return it->second;
}

void Variables::unstore(const std::string& name) {
    vars_.erase(name);
}

void Variables::store_quick(int n, ValuePtr value) {
    if (n < 0 || n > 9) throw std::out_of_range("quick register must be 0-9");
    quick_[n] = std::move(value);
}

std::optional<ValuePtr> Variables::recall_quick(int n) const {
    if (n < 0 || n > 9) throw std::out_of_range("quick register must be 0-9");
    if (!quick_[n]) return std::nullopt;
    return quick_[n];
}

void Variables::store_add(const std::string& name, ValuePtr value, int precision) {
    auto cur = recall(name);
    if (!cur) throw std::invalid_argument("variable '" + name + "' not stored");
    store(name, arith::add(*cur, value, precision));
}

void Variables::store_sub(const std::string& name, ValuePtr value, int precision) {
    auto cur = recall(name);
    if (!cur) throw std::invalid_argument("variable '" + name + "' not stored");
    store(name, arith::sub(*cur, value, precision));
}

void Variables::store_mul(const std::string& name, ValuePtr value, int precision) {
    auto cur = recall(name);
    if (!cur) throw std::invalid_argument("variable '" + name + "' not stored");
    store(name, arith::mul(*cur, value, precision));
}

void Variables::store_div(const std::string& name, ValuePtr value, int precision) {
    auto cur = recall(name);
    if (!cur) throw std::invalid_argument("variable '" + name + "' not stored");
    store(name, arith::div(*cur, value, precision));
}

ValuePtr Variables::exchange(const std::string& name, ValuePtr value) {
    auto cur = recall(name);
    if (!cur) throw std::invalid_argument("variable '" + name + "' not stored");
    auto old = *cur;
    store(name, std::move(value));
    return old;
}

bool Variables::exists(const std::string& name) const {
    return vars_.find(name) != vars_.end();
}

std::vector<std::string> Variables::names() const {
    std::vector<std::string> result;
    result.reserve(vars_.size());
    for (auto& [k, v] : vars_) result.push_back(k);
    return result;
}

} // namespace sc
