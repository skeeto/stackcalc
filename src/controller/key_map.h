#pragma once

#include <string>
#include <unordered_map>
#include <optional>

namespace sc {

// Maps key names to command names.
// Supports simple one-key mappings and prefix sequences.
class KeyMap {
public:
    KeyMap();

    // Look up a key. Returns command name if found.
    std::optional<std::string> lookup(const std::string& key) const;

    // Check if key is a prefix (more keys expected)
    bool is_prefix(const std::string& key) const;

    // Look up a two-key sequence (prefix + key)
    std::optional<std::string> lookup_seq(const std::string& prefix, const std::string& key) const;

    // Register a binding
    void bind(const std::string& key, const std::string& command);
    void bind_seq(const std::string& prefix, const std::string& key, const std::string& command);

private:
    void setup_defaults();

    std::unordered_map<std::string, std::string> simple_;
    std::unordered_map<std::string, std::unordered_map<std::string, std::string>> prefix_;
};

} // namespace sc
