#pragma once

#include <string>

namespace tool::env {
    // Loads environment variables from a file (defaults to ".env")
    void load(const std::string& filepath = ".env");

    // Fetches configuration value. Checks system environment first, then loaded map, with fallback to default.
    std::string get(const std::string& key, const std::string& defaultValue = "");
}
