#pragma once
#include "identity/Auth/Auth.h"
#include <string>
#include <vector>

namespace tool::ui {
    void displayTitle(std::string_view title, int width = 64);

    // Prints a row with specific column widths
    // Example: printRow({4, 20, 15}, {"ID", "Name", "Category"})
    void printRow(const std::vector<int>& widths, const std::vector<std::string>& values);

    // Aligns labels to the right and values to the left
    // Example: printField("Email Address", "zero@utem.edu.my")
    void printField(std::string_view label, std::string_view value, int labelWidth = 15);

    // Formats a double into "RM 0.00" with specific padding
    void printPrice(std::string_view label, double amount, int width = 10);

    // Clears the screen, draws top separator, centers title, draws bottom separator, and prints a spacer line
    void showHeader(std::string_view title, int width = 64, char decoration = '=');

    // Increments the invalid attempts counter, checks if max attempts are reached,
    // sleeps for pauseSeconds if triggered, and returns true if limit was hit.
    bool handleInvalidAttempt(int& invalidAttempts, int maxAttempts = 3, int pauseSeconds = 5);

    // Prints a return line prompt and loops until '0' is entered
    void pressZeroToReturn(std::string_view destination = "dashboard", int width = 64);

    // Standardized printout of pagination status and keys
    void printPaginationFooter(int currentPage, int totalPages, int totalItems, int width = 90);
}
