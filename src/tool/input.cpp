#include "tool/input.h"
#include <iostream>

using namespace std;

namespace tool::input {
    bool readInt(int& value) {
        if (cin >> value) {
            cin.ignore(1000, '\n');
            return true;
        }
        cin.clear();
        cin.ignore(1000, '\n');
        return false;
    }

    bool readLine(string& value) {
        return static_cast<bool>(getline(cin, value));
    }
}