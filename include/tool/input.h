#pragma once
#include <string>

namespace tool::input {
    // Safely reads an integer from standard input, clearing the stream and ignoring invalid entries.
    // Returns true on success, false on failure (invalid input).
    bool readInt(int& value);

    // Safely reads a full line of string input.
    // Returns true on success, false on EOF/failure.
    bool readLine(std::string& value);
}
