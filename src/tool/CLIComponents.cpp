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
            int colWidth = widths[i] < 0 ? 0 : widths[i];
            size_t w = static_cast<size_t>(colWidth);
            string padded = values[i];
            
            if (padded.length() > w) {
                if (w > 3) {
                    padded = padded.substr(0, w - 3) + "...";
                } else {
                    padded = padded.substr(0, w);
                }
            } else if (padded.length() < w) {
                padded.append(w - padded.length(), ' ');
            }
            row += padded;
            
            if (i < values.size() - 1) {
                row += " | ";
            }
        }
        println("{}", row);
    }
    
    void printField(string_view label, string_view value, int labelWidth) {
        // Manually construct padding for right-aligned label
        string paddedLabel(label);
        if (static_cast<int>(paddedLabel.length()) < labelWidth) {
            paddedLabel.insert(0, labelWidth - paddedLabel.length(), ' ');
        }
        println("  {} : {}", paddedLabel, value);
    }

    void printPrice(string_view label, double amount, int width) {
        string priceStr = format("RM {:.2f}", amount);
        // Manually construct padding for right-aligned price
        if (static_cast<int>(priceStr.length()) < width) {
            priceStr.insert(0, width - priceStr.length(), ' ');
        }
        println("  {:<15} : {}", label, priceStr);
    }

    void showHeader(std::string_view title, int width, char decoration) {
        tool::helper::clearScreen();
        tool::helper::drawLine(width, decoration);
        displayTitle(title, width);
        tool::helper::drawLine(width, decoration);
        println("");
    }

    bool handleInvalidAttempt(int& invalidAttempts, int maxAttempts, int pauseSeconds) {
        invalidAttempts++;
        if (invalidAttempts >= maxAttempts) {
            println("\nToo many invalid attempts. Pausing for {} seconds...", pauseSeconds);
            std::this_thread::sleep_for(std::chrono::seconds(pauseSeconds));
            invalidAttempts = 0;
            return true;
        }
        return false;
    }

    void pressZeroToReturn(std::string_view destination, int width) {
        println("");
        tool::helper::drawLine(width, '-');
        std::string waitInput;
        do {
            print("\nEnter '0' to return to {}: ", destination);
            if (!std::getline(std::cin, waitInput)) {
                break;
            }
        } while (waitInput != "0");
    }

    void printPaginationFooter(int currentPage, int totalPages, int totalItems, int width) {
        if (totalPages > 1) {
            string pageInfo = format("Page {} of {} | Total: {}", currentPage, totalPages, totalItems);
            int padding = (width - static_cast<int>(pageInfo.length())) / 2;
            string spaces(padding > 0 ? padding : 0, ' ');
            println("{}{}", spaces, pageInfo);
            tool::helper::drawLine(width, '-');
            println("  [N] Next Page      [P] Previous Page");
        }
    }
}
