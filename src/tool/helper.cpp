#include "tool/helper.h"
#include <cstdlib>
#include <iostream>
#include <print>
#include <string>

using namespace std;

namespace tool::helper {
    // Helper to clear terminal for a professional presentation
    void clearScreen() {
        #ifdef _WIN32
            system("cls");
        #else
            system("clear");
        #endif
    }

    // Helper to draw a full line of symbols
    void drawLine(int width, char symbol) {
        println("{}", string(width, symbol));
    }
}