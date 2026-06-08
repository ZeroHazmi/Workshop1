#include "tool/input.h"
#include <iostream>

namespace tool::input {
    bool readInt(int& value) {
        if (std::cin >> value) {
            std::cin.ignore(1000, '\n');
            return true;
        }
        std::cin.clear();
        std::cin.ignore(1000, '\n');
        return false;
    }

    bool readLine(std::string& value) {
        return static_cast<bool>(std::getline(std::cin, value));
    }
}
