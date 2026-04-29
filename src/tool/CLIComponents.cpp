#include <print>
#include <string>
#include <iostream>
#include <thread>

#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "identity/Auth/Auth.h"

using namespace std;

namespace tool::ui {

    // The Header Function
    void displayTitle(string_view title, int width) {
        // Calculate padding for centering
        int titleLen = static_cast<int>(title.length());
        int padding = (width - 2 - titleLen) / 2; // -2 for the side borders '|'
        
        // Ensure we don't have negative padding if title is too long
        if (padding < 0) padding = 0;

        // Construct the centered line
        string leftPad(padding, ' ');
        // If width is odd and title is even (or vice versa), add extra space to the right
        string rightPad(width - 2 - titleLen - padding, ' ');

        println("|{}{}{}|", leftPad, title, rightPad);        
    }

    void printRow(const vector<int>& widths, const vector<string>& values) {
        string row = " ";
        for (size_t i = 0; i < values.size(); ++i) {
            // Manually pad the string to the specified width
            string padded = values[i];
            if (padded.length() < static_cast<size_t>(widths[i])) {
                padded.append(widths[i] - padded.length(), ' ');
            }
            row += padded;
            
            if (i < values.size() - 1) {
                row += " | ";
            }
        }
        println("{}", row);
    }
    
    void printField(std::string_view label, std::string_view value, int labelWidth) {
        // Manually construct padding for right-aligned label
        string paddedLabel(label);
        if (static_cast<int>(paddedLabel.length()) < labelWidth) {
            paddedLabel.insert(0, labelWidth - paddedLabel.length(), ' ');
        }
        std::println("  {} : {}", paddedLabel, value);
    }

    void printPrice(std::string_view label, double amount, int width) {
        std::string priceStr = std::format("RM {:.2f}", amount);
        // Manually construct padding for right-aligned price
        if (static_cast<int>(priceStr.length()) < width) {
            priceStr.insert(0, width - priceStr.length(), ' ');
        }
        std::println("  {:<15} : {}", label, priceStr);
    }
}
