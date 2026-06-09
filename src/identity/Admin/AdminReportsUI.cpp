#include "identity/AdminUI.h"
#include "identity/Admin/Reports/ReportsInternal.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include "transaction/Rental/AdminStats.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <print>
#include <string>
#include <iostream>
#include <thread>
#include <format>
#include <sstream>
#include <vector>

using namespace std;

namespace identity::adminui {

    bool isValidDate(const std::string& dateStr) {
        if (dateStr.length() != 10) return false;
        if (dateStr[4] != '-' || dateStr[7] != '-') return false;
        for (int i = 0; i < 10; ++i) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(dateStr[i])) return false;
        }
        return true;
    }

    void updateActiveDateRange(transaction::rental::stats::DateRange& activeDateRange, std::string& activeDateRangeLabel) {
        println("");
        println("  Select Time Range Option:");
        println("  [1] Last 3 Months");
        println("  [2] Last 6 Months (Default)");
        println("  [3] Last 12 Months");
        println("  [4] Custom Date Range");
        println("  [0] Clear Date Filter (All Time)");
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
        
        string choice;
        getline(cin, choice);
        
        auto& db = database::DatabaseManager::getInstance();
        
        if (choice == "1") {
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 3 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                    activeDateRangeLabel = "Last 3 Months";
                    print("\n  Filter updated to: Last 3 Months ({} to {})\n", activeDateRange.startDate, activeDateRange.endDate);
                }
                delete rs;
            } else {
                print("\n  [Error] Failed to calculate dates in database: {}\n", result.error());
            }
        } else if (choice == "2") {
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 6 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                    activeDateRangeLabel = "Last 6 Months";
                    print("\n  Filter updated to: Last 6 Months ({} to {})\n", activeDateRange.startDate, activeDateRange.endDate);
                }
                delete rs;
            } else {
                print("\n  [Error] Failed to calculate dates in database: {}\n", result.error());
            }
        } else if (choice == "3") {
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 12 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                    activeDateRangeLabel = "Last 12 Months";
                    print("\n  Filter updated to: Last 12 Months ({} to {})\n", activeDateRange.startDate, activeDateRange.endDate);
                }
                delete rs;
            } else {
                print("\n  [Error] Failed to calculate dates in database: {}\n", result.error());
            }
        } else if (choice == "4") {
            print("  Enter Start Date (YYYY-MM-DD): ");
            string startInput;
            getline(cin, startInput);
            
            print("  Enter End Date (YYYY-MM-DD): ");
            string endInput;
            getline(cin, endInput);
            
            // Trim inputs
            startInput.erase(0, startInput.find_first_not_of(" \t\r\n"));
            startInput.erase(startInput.find_last_not_of(" \t\r\n") + 1);
            endInput.erase(0, endInput.find_first_not_of(" \t\r\n"));
            endInput.erase(endInput.find_last_not_of(" \t\r\n") + 1);
            
            if (!isValidDate(startInput) || !isValidDate(endInput)) {
                println("\n  [Error] Invalid date format. Must be strictly YYYY-MM-DD.");
            } else if (startInput > endInput) {
                println("\n  [Error] Start date cannot be after End date.");
            } else {
                activeDateRange.startDate = startInput;
                activeDateRange.endDate = endInput;
                activeDateRangeLabel = "Custom (" + startInput + " to " + endInput + ")";
                print("\n  Filter updated to: Custom range ({} to {})\n", startInput, endInput);
            }
        } else if (choice == "0") {
            activeDateRange.startDate = "";
            activeDateRange.endDate = "";
            activeDateRangeLabel = "All Time";
            println("\n  Date range filter cleared (All Time).");
        } else {
            println("\n  Invalid option. Preserving previous timeframe.");
        }
        
        this_thread::sleep_for(chrono::milliseconds(1500));
    }

    void updateActiveFilters(std::vector<int>& activeShopIds, std::vector<std::string>& activeShopNames) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = "SELECT shop_id, unique_id, shop_name FROM shops ORDER BY shop_id ASC;";
        auto result = db.executeQuery(query);
        println("");
        if (result) {
            sql::ResultSet* rs = result.value();
            print("  Available Shops: ");
            bool first = true;
            while (rs->next()) {
                if (!first) print(" | ");
                print("{}: {}", string(rs->getString("unique_id")), string(rs->getString("shop_name")));
                first = false;
            }
            println("");
            delete rs;
        }
        
        print("  Enter Shop ID(s) to filter by, separated by commas (e.g. SHP-9, SHP-A8X92E or 0 for All): ");
        string input;
        getline(cin, input);
        
        // Parse input
        std::vector<string> newUniqueIds;
        std::stringstream ss(input);
        std::string token;
        bool hasZero = false;
        while (std::getline(ss, token, ',')) {
            // Trim token
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            if (token.empty()) continue;
            if (token == "0") {
                hasZero = true;
            } else {
                newUniqueIds.push_back(token);
            }
        }
        
        if (hasZero || newUniqueIds.empty()) {
            activeShopIds.clear();
            activeShopNames.clear();
            println("\n  Filters cleared. Showing all shops.");
        } else {
            // Validate and fetch shop names
            std::vector<int> validatedIds;
            std::vector<string> validatedNames;
            for (const string& uid : newUniqueIds) {
                std::string checkQuery = "SELECT shop_id, shop_name FROM shops WHERE unique_id = '" + uid + "' OR shop_id = '" + uid + "';";
                auto checkRes = db.executeQuery(checkQuery);
                if (checkRes) {
                    sql::ResultSet* crs = checkRes.value();
                    if (crs->next()) {
                        validatedIds.push_back(crs->getInt("shop_id"));
                        validatedNames.push_back(crs->getString("shop_name"));
                    }
                    delete crs;
                }
            }
            activeShopIds = validatedIds;
            activeShopNames = validatedNames;
            if (activeShopIds.empty()) {
                println("\n  No valid Shop IDs entered. Filters cleared.");
            } else {
                println("");
                print("  Active Filter set to: ");
                for (size_t i = 0; i < activeShopNames.size(); ++i) {
                    print("{}", activeShopNames[i]);
                    if (i + 1 < activeShopNames.size()) print(", ");
                }
                println("");
            }
        }
        this_thread::sleep_for(chrono::milliseconds(1500));
    }

    void showBusinessStatsSubmenu(const ::identity::auth::UserSession& session) {
        std::vector<int> activeShopIds;
        std::vector<string> activeShopNames;
        
        transaction::rental::stats::DateRange activeDateRange;
        std::string activeDateRangeLabel = "Last 6 Months";
        
        // Initialize default to Last 6 Months dynamically on first load
        {
            auto& db = database::DatabaseManager::getInstance();
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 6 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                }
                delete rs;
            }
        }
        
        bool inStats = true;
        int invalidAttempts = 0;
        while (inStats) {
            tool::ui::showHeader("BUSINESS REPORTS & STATISTICS", 64);
            
            // Show active filters
            if (activeShopIds.empty()) {
                print("  Active Filter: All Shops");
            } else {
                print("  Active Filter: ");
                for (size_t i = 0; i < activeShopNames.size(); ++i) {
                    print("{}", activeShopNames[i]);
                    if (i + 1 < activeShopNames.size()) print(", ");
                }
            }
            print(" | Time Range: {}\n", activeDateRangeLabel);
            println("");
            
            println("  [1] Monthly Revenue Trends");
            println("  [2] Costume Popularity & Demand");
            println("  [3] Branch Performance & Revenue Share");
            println("  [4] Inventory Quality Audit");
            println("  [5] View Invoice Ledger");
            println("  [F] Toggle/Edit Branch Filters");
            println("  [T] Toggle/Edit Time Range");
            println("  [0] Return to Dashboard");
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");

            string choiceStr;
            getline(cin, choiceStr);
            // Trim choiceStr
            choiceStr.erase(0, choiceStr.find_first_not_of(" \t\r\n"));
            choiceStr.erase(choiceStr.find_last_not_of(" \t\r\n") + 1);

            if (choiceStr == "1") {
                invalidAttempts = 0;
                reports::showMonthlyRevenueReport(activeShopIds, activeShopNames, activeDateRange, activeDateRangeLabel);
            } else if (choiceStr == "2") {
                invalidAttempts = 0;
                reports::showCostumePopularityReport(activeShopIds, activeShopNames, activeDateRange, activeDateRangeLabel);
            } else if (choiceStr == "3") {
                invalidAttempts = 0;
                reports::showBranchPerformanceReport(activeShopIds, activeShopNames, activeDateRange, activeDateRangeLabel);
            } else if (choiceStr == "4") {
                invalidAttempts = 0;
                reports::showInventoryQualityReport(activeShopIds, activeShopNames);
            } else if (choiceStr == "5") {
                invalidAttempts = 0;
                reports::showInvoiceLedgerReport(activeShopIds, activeShopNames);
            } else if (choiceStr == "F" || choiceStr == "f") {
                invalidAttempts = 0;
                updateActiveFilters(activeShopIds, activeShopNames);
            } else if (choiceStr == "T" || choiceStr == "t") {
                invalidAttempts = 0;
                updateActiveDateRange(activeDateRange, activeDateRangeLabel);
            } else if (choiceStr == "0") {
                invalidAttempts = 0;
                inStats = false;
            } else {
                println("  Invalid option.");
                if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            }
        }
    }
}
