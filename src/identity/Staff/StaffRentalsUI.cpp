#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/DateHelper.h"
#include "transaction/Rental/Rental.h"
#include <print>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <format>
#include <algorithm>

using namespace std;

namespace identity::staffui {

    void manageRentalsAndReturns(const ::identity::auth::UserSession& session) {
        bool inRentalsMenu = true;
        string searchTerm = "";
        int currentPage = 1;
        const int itemsPerPage = 25;
        int invalidAttempts = 0;

        while (inRentalsMenu) {
            tool::ui::showHeader("ACTIVE & OVERDUE RENTALS MANAGEMENT", 85);
            if (!searchTerm.empty()) {
                println("  [Search Filter: '{}']", searchTerm);
            }
            println("");

            // Header for active rentals table
            vector<int> colWidths = {12, 18, 18, 5, 12, 10, 10};
            tool::ui::printRow(colWidths, {"ID", "CUSTOMER", "ITEM RENTED", "SIZE", "RETURN DATE", "RATE/DAY", "STATUS"});
            tool::helper::drawLine(85, '-');

            auto result = ::transaction::rental::getActiveRentals(searchTerm);
            if (result) {
                const auto& allItems = result.value();
                int totalItems = static_cast<int>(allItems.size());
                int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;

                if (currentPage > totalPages) currentPage = totalPages;
                if (currentPage < 1) currentPage = 1;

                int startIndex = (currentPage - 1) * itemsPerPage;
                int endIndex = min(startIndex + itemsPerPage, totalItems);

                if (totalItems == 0) {
                    println("  No active rentals found.");
                } else {
                    for (int i = startIndex; i < endIndex; ++i) {
                        const auto& item = allItems[i];
                        string statusStr = item.is_overdue ? "OVERDUE" : "Active";
                        tool::ui::printRow(colWidths, {
                            item.unique_id,
                            item.customer_name,
                            item.item_name,
                            item.size,
                            item.expected_return_date,
                            format("RM {:.2f}", item.daily_rate),
                            statusStr
                        });
                    }
                }

                tool::helper::drawLine(85, '=');
                
                // Pagination and Options Info
                if (totalPages > 1) {
                    string pageInfo = format("Page {} of {} | Total Outstanding: {}", currentPage, totalPages, totalItems);
                    int padding = (85 - static_cast<int>(pageInfo.length())) / 2;
                    string spaces(padding > 0 ? padding : 0, ' ');
                    println("{}{}", spaces, pageInfo);
                    tool::helper::drawLine(85, '-');
                    println("  [N] Next Page      [P] Previous Page      [S] Search Rentals");
                } else {
                    println("  [S] Search Rentals");
                }
                println("  [R] Process Return/Check-in Costume");
                println("  [0] Back to Main Dashboard");
                tool::helper::drawLine(85, '-');
                print("  Enter choice: ");

                string input;
                getline(cin, input);

                if (input == "0") {
                    invalidAttempts = 0;
                    inRentalsMenu = false;
                } else if (input == "N" || input == "n") {
                    invalidAttempts = 0;
                    if (currentPage < totalPages) currentPage++;
                } else if (input == "P" || input == "p") {
                    invalidAttempts = 0;
                    if (currentPage > 1) currentPage--;
                } else if (input == "S" || input == "s") {
                    invalidAttempts = 0;
                    print("  Enter search query (Customer or Apparel name): ");
                    getline(cin, searchTerm);
                    currentPage = 1;
                } else if (input == "R" || input == "r") {
                    invalidAttempts = 0;
                    // Inline Check-in costume return flow!
                    println("\n  --- PROCESS COSTUME RETURN ---");
                    string rentalIdInput;
                    print("  Enter Rental ID to process (or 0 to cancel): ");
                    getline(cin, rentalIdInput);
                    if (rentalIdInput.empty() || rentalIdInput == "0") {
                        continue;
                    }

                    // Prompt for actual return date with format validation
                    string returnDate;
                    while (true) {
                        print("  Enter Actual Return Date (DD/MM/YYYY) (or '0' to cancel): ");
                        getline(cin, returnDate);
                        if (returnDate == "0") break;
                        if (tool::date::isValidFormat(returnDate)) {
                            returnDate = tool::date::normalizeDateStr(returnDate);
                            break;
                        }
                        println("  [Error] Invalid date format. Please use DD/MM/YYYY.");
                    }
                    if (returnDate == "0") continue;

                    // Prompt for condition (Excellent/Good/Fair/Poor/Damaged)
                    string condition;
                    while (true) {
                        print("  Enter Return Condition (Excellent/Good/Fair/Poor/Damaged) (or '0' to cancel): ");
                        getline(cin, condition);
                        if (condition == "0") break;
                        
                        // normalize
                        if (condition == "excellent" || condition == "Excellent") condition = "Excellent";
                        else if (condition == "good" || condition == "Good") condition = "Good";
                        else if (condition == "fair" || condition == "Fair") condition = "Fair";
                        else if (condition == "poor" || condition == "Poor") condition = "Poor";
                        else if (condition == "damaged" || condition == "Damaged") condition = "Damaged";
                        else {
                            println("  [Error] Invalid condition. Use Excellent/Good/Fair/Poor/Damaged.");
                            continue;
                        }
                        break;
                    }
                    if (condition == "0") continue;

                    println("\n  Processing return...");
                    auto returnRes = ::transaction::rental::processCostumeReturn(rentalIdInput, returnDate, condition);
                    if (returnRes) {
                        println("{}", returnRes.value());
                    } else {
                        println("  [Error] Failed to process return: {}", returnRes.error());
                    }
                    tool::ui::pressZeroToReturn("previous menu", 85);
                } else {
                    println("  Invalid choice.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
                }
            } else {
                println("  Error retrieving active rentals: {}", result.error());
                tool::ui::pressZeroToReturn("dashboard", 85);
                inRentalsMenu = false;
            }
        }
    }

}
