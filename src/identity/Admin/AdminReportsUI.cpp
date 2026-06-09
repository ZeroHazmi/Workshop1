#include "identity/AdminUI.h"
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

    static bool isValidDate(const std::string& dateStr) {
        if (dateStr.length() != 10) return false;
        if (dateStr[4] != '-' || dateStr[7] != '-') return false;
        for (int i = 0; i < 10; ++i) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(dateStr[i])) return false;
        }
        return true;
    }

    static void updateActiveDateRange(transaction::rental::stats::DateRange& activeDateRange, std::string& activeDateRangeLabel) {
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

    static void updateActiveFilters(std::vector<int>& activeShopIds, std::vector<string>& activeShopNames) {
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
                bool inReport = true;
                while (inReport) {
                    tool::ui::showHeader("MONTHLY REVENUE TRENDS", 64);
                    
                    // Header Filter Status
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

                    auto trendsOpt = transaction::rental::stats::getRevenueTrends(activeShopIds, activeDateRange);
                    if (!trendsOpt) {
                        print("{}\n", format("  Error retrieving revenue trends: {}", trendsOpt.error()));
                    } else {
                        auto trends = trendsOpt.value();
                        if (trends.empty()) {
                            println("  No revenue data available for selected timeframe.");
                        } else {
                            // Find max revenue for scaling
                            double maxRev = 0.0;
                            for (const auto& pt : trends) {
                                if (pt.revenue > maxRev) maxRev = pt.revenue;
                            }

                            // Print vertical bar chart
                            println("  Total (RM)");
                            int maxRows = 10;
                            const std::vector<std::string> barColors = {
                                "\033[96m", // Bright Cyan
                                "\033[95m", // Bright Magenta
                                "\033[93m", // Bright Yellow
                                "\033[94m", // Bright Blue
                                "\033[92m", // Bright Green
                                "\033[91m", // Bright Red
                                "\033[36m", // Cyan
                                "\033[35m", // Magenta
                                "\033[33m", // Yellow
                                "\033[34m", // Blue
                                "\033[32m", // Green
                                "\033[31m"  // Red
                            };
                            for (int r = maxRows; r >= 1; --r) {
                                double threshold = maxRev * (r / (double)maxRows);
                                std::print("  {:9.2f} | ", threshold);
                                size_t cIdx = 0;
                                for (const auto& pt : trends) {
                                    std::string color = barColors[cIdx % barColors.size()];
                                    cIdx++;
                                    if (pt.revenue >= threshold && pt.revenue > 0.0) {
                                        std::print("   {}{}{}   ", color, "███", "\033[0m");
                                    } else {
                                        std::print("         ");
                                    }
                                }
                                std::print("\n");
                            }

                            // Chart base line
                            std::print("            +");
                            for (size_t i = 0; i < trends.size(); ++i) {
                                std::print("---------");
                            }
                            std::print("\n             ");

                            // X-axis month labels formatted to exactly 6 characters (e.g., "May 26") and centered
                            for (const auto& pt : trends) {
                                std::string displayMonth = pt.month_name;
                                if (displayMonth.length() == 8 && displayMonth[3] == ' ') {
                                    displayMonth = displayMonth.substr(0, 3) + " " + displayMonth.substr(6, 2);
                                }
                                std::print("  {:^6} ", displayMonth);
                            }
                            print("\n\n");

                            // Print X-axis label centered
                            int xLabelWidth = 9 * static_cast<int>(trends.size());
                            int xLabelPad = 13 + (xLabelWidth - 6) / 2;
                            if (xLabelPad < 0) xLabelPad = 0;
                            println("{}{}", string(xLabelPad, ' '), "Months");
                            println("");

                            // Tabular details
                            tool::helper::drawLine(64, '-');
                            print("{}\n", format("  {:<20} | {:<20}", "MONTH", "REVENUE (RM)"));
                            tool::helper::drawLine(64, '-');
                            for (const auto& pt : trends) {
                                print("{}\n", format("  {:<20} | RM {:<18.2f}", pt.month_name, pt.revenue));
                            }
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops, 'T' to change time range: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    } else if (waitInput == "T" || waitInput == "t") {
                        updateActiveDateRange(activeDateRange, activeDateRangeLabel);
                    }
                }
            } else if (choiceStr == "2") {
                invalidAttempts = 0;
                bool inReport = true;
                while (inReport) {
                    tool::ui::showHeader("COSTUME POPULARITY & DEMAND", 64);
                    
                    // Header Filter Status
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

                    auto popOpt = transaction::rental::stats::getCostumePopularity(activeShopIds, activeDateRange);
                    if (!popOpt) {
                        print("{}\n", format("  Error retrieving costume popularity: {}", popOpt.error()));
                    } else {
                        auto points = popOpt.value();
                        if (points.empty()) {
                            println("  No rental data available for selected timeframe.");
                        } else {
                            int maxCount = 0;
                            for (const auto& pt : points) {
                                if (pt.rental_count > maxCount) maxCount = pt.rental_count;
                            }

                            size_t maxCatalogNameLen = 25; // default minimum
                            for (const auto& pt : points) {
                                if (pt.catalog_name.length() > maxCatalogNameLen) {
                                    maxCatalogNameLen = pt.catalog_name.length();
                                }
                            }

                            println("  Top Popular Costumes by Rental Volume:");
                            println("");

                            int maxBarWidth = 25;
                            const std::vector<std::string> barColors = {
                                "\033[96m", // Bright Cyan
                                "\033[95m", // Bright Magenta
                                "\033[93m", // Bright Yellow
                                "\033[94m", // Bright Blue
                                "\033[92m", // Bright Green
                                "\033[91m", // Bright Red
                                "\033[36m", // Cyan
                                "\033[35m", // Magenta
                                "\033[33m", // Yellow
                                "\033[34m", // Blue
                                "\033[32m", // Green
                                "\033[31m"  // Red
                            };
                            size_t cIdx = 0;
                            for (const auto& pt : points) {
                                int barWidth = maxCount > 0 ? (pt.rental_count * maxBarWidth / maxCount) : 0;
                                string unicodeBar = "";
                                for (int i = 0; i < barWidth; ++i) {
                                    unicodeBar += "█";
                                }
                                string spaces(maxBarWidth - barWidth, ' ');
                                string color = barColors[cIdx % barColors.size()];
                                cIdx++;
                                std::print("  {:<{}} | {}{}{} ({} rentals)\n", 
                                           pt.catalog_name, maxCatalogNameLen, color, unicodeBar + spaces, "\033[0m", pt.rental_count);
                            }
                            println("");
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops, 'T' to change time range: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    } else if (waitInput == "T" || waitInput == "t") {
                        updateActiveDateRange(activeDateRange, activeDateRangeLabel);
                    }
                }
            } else if (choiceStr == "3") {
                invalidAttempts = 0;
                bool inReport = true;
                while (inReport) {
                    tool::ui::showHeader("BRANCH PERFORMANCE & REVENUE SHARE", 64);
                    
                    // Header Filter Status
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

                    auto branchOpt = transaction::rental::stats::getBranchPerformance(activeShopIds, activeDateRange);
                    if (!branchOpt) {
                        print("{}\n", format("  Error retrieving branch performance: {}", branchOpt.error()));
                    } else {
                        auto branches = branchOpt.value();
                        if (branches.empty()) {
                            println("  No branch revenue records found for selected timeframe.");
                        } else {
                            double totalRevenue = 0.0;
                            for (const auto& pt : branches) totalRevenue += pt.revenue;

                            print("{}\n\n", format("  Total Combined Revenue: RM {:.2f}", totalRevenue));
                            
                            if (totalRevenue > 0.0) {
                                const std::vector<std::string> barColors = {
                                    "\033[96m", // Bright Cyan
                                    "\033[95m", // Bright Magenta
                                    "\033[93m", // Bright Yellow
                                    "\033[94m", // Bright Blue
                                    "\033[92m", // Bright Green
                                    "\033[91m", // Bright Red
                                    "\033[36m", // Cyan
                                    "\033[35m", // Magenta
                                    "\033[33m", // Yellow
                                    "\033[34m", // Blue
                                    "\033[32m", // Green
                                    "\033[31m"  // Red
                                };
                                size_t cIdx = 0;
                                for (const auto& pt : branches) {
                                    double pct = (pt.revenue / totalRevenue) * 100.0;
                                    int barChars = (int)(pct / 2.5); // 40 chars total
                                    string barStr = "";
                                    for (int i = 0; i < barChars; ++i) barStr += "█";
                                    string remStr = "";
                                    for (int i = 0; i < (40 - barChars); ++i) remStr += "░";
                                    
                                    string color = barColors[cIdx % barColors.size()];
                                    cIdx++;
                                    
                                    std::print("  {:<20} [ {}{}{}{} ] {:5.1f}% (RM {:.2f})\n", 
                                        pt.shop_name, color, barStr, "\033[0m", remStr, pct, pt.revenue);
                                }
                            } else {
                                println("  No revenue generated yet.");
                            }
                            println("");
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops, 'T' to change time range: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    } else if (waitInput == "T" || waitInput == "t") {
                        updateActiveDateRange(activeDateRange, activeDateRangeLabel);
                    }
                }
            } else if (choiceStr == "4") {
                invalidAttempts = 0;
                bool inReport = true;
                while (inReport) {
                    tool::ui::showHeader("INVENTORY QUALITY AUDIT", 64);
                    
                    // Header Filter Status (Time Range N/A since physical condition represents immediate current state)
                    if (activeShopIds.empty()) {
                        print("  Active Filter: All Shops");
                    } else {
                        print("  Active Filter: ");
                        for (size_t i = 0; i < activeShopNames.size(); ++i) {
                            print("{}", activeShopNames[i]);
                            if (i + 1 < activeShopNames.size()) print(", ");
                        }
                    }
                    print(" | Time Range: N/A (Current Physical State)\n");
                    println("");

                    auto auditOpt = transaction::rental::stats::getInventoryConditionAudit(activeShopIds);
                    if (!auditOpt) {
                        print("{}\n", format("  Error retrieving inventory audit: {}", auditOpt.error()));
                    } else {
                        auto conditions = auditOpt.value();
                        if (conditions.empty()) {
                            println("  No physical inventory items found in database.");
                        } else {
                            int totalItems = 0;
                            for (const auto& pt : conditions) totalItems += pt.count;

                            vector<int> colWidths = {25, 15, 15};
                            tool::helper::drawLine(64, '-');
                            tool::ui::printRow(colWidths, {"CONDITION STATUS", "ITEM COUNT", "PERCENTAGE"});
                            tool::helper::drawLine(64, '-');

                            for (const auto& pt : conditions) {
                                double pct = totalItems > 0 ? (pt.count / (double)totalItems) * 100.0 : 0.0;
                                string countStr = to_string(pt.count);
                                string pctStr = format("{:.1f}%", pct);
                                tool::ui::printRow(colWidths, {pt.condition, countStr, pctStr});
                            }
                            tool::helper::drawLine(64, '-');
                            string totalItemsStr = to_string(totalItems);
                            tool::ui::printRow(colWidths, {"TOTAL ITEMS", totalItemsStr, "100.0%"});
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    }
                }
            } else if (choiceStr == "5") {
                invalidAttempts = 0;
                bool inLedger = true;
                string statusFilter = "ALL"; // ALL, Paid, Settled, Overdue
                int currentPage = 1;
                const int itemsPerPage = 15;
                int invalidAttemptsLedger = 0;

                while (inLedger) {
                    tool::ui::showHeader("INVOICE LEDGER & AUDITING", 118);

                    // Filter Status
                    if (activeShopIds.empty()) {
                        print("  Active Filter: All Shops");
                    } else {
                        print("  Active Filter: ");
                        for (size_t i = 0; i < activeShopNames.size(); ++i) {
                            print("{}", activeShopNames[i]);
                            if (i + 1 < activeShopNames.size()) print(", ");
                        }
                    }
                    print(" | Status: {}\n", statusFilter);
                    println("");

                    auto& db = database::DatabaseManager::getInstance();

                    // Count total matching records for pagination
                    std::string countQuery = "SELECT COUNT(*) FROM invoices inv JOIN rental r ON inv.rental_id = r.rental_id WHERE 1=1";
                    if (!activeShopIds.empty()) {
                        std::string shopFilterStr = "";
                        for (size_t i = 0; i < activeShopIds.size(); ++i) {
                            shopFilterStr += to_string(activeShopIds[i]);
                            if (i + 1 < activeShopIds.size()) shopFilterStr += ",";
                        }
                        countQuery += std::format(" AND r.shop_id IN ({})", shopFilterStr);
                    }
                    if (statusFilter != "ALL") {
                        countQuery += std::format(" AND inv.payment_status = '{}'", statusFilter);
                    }

                    int totalItems = 0;
                    auto countRes = db.executeQuery(countQuery);
                    if (countRes) {
                        sql::ResultSet* rs = countRes.value();
                        if (rs->next()) {
                            totalItems = rs->getInt(1);
                        }
                        delete rs;
                    }

                    int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;
                    if (currentPage > totalPages) currentPage = totalPages;
                    if (currentPage < 1) currentPage = 1;

                    // Header
                    std::vector<int> colWidths = {12, 12, 18, 9, 9, 9, 9, 18};
                    tool::ui::printRow(colWidths, {"INV ID", "RENTAL ID", "CUSTOMER NAME", "BASE", "DEPOSIT", "LATE FEE", "TOTAL DUE", "STATUS"});
                    tool::helper::drawLine(118, '-');

                    // Fetch records
                    int offset = (currentPage - 1) * itemsPerPage;
                    std::string selectQuery = 
                        "SELECT inv.unique_id AS inv_uid, r.unique_id AS rnt_uid, cust.fullname AS customer_name, "
                        "       inv.base_fee, inv.security_deposit, inv.late_fee, inv.total_amount, inv.payment_status "
                        "FROM invoices inv "
                        "JOIN rental r ON inv.rental_id = r.rental_id "
                        "JOIN customers cust ON r.cust_id = cust.cust_id "
                        "WHERE 1=1";

                    if (!activeShopIds.empty()) {
                        std::string shopFilterStr = "";
                        for (size_t i = 0; i < activeShopIds.size(); ++i) {
                            shopFilterStr += to_string(activeShopIds[i]);
                            if (i + 1 < activeShopIds.size()) shopFilterStr += ",";
                        }
                        selectQuery += std::format(" AND r.shop_id IN ({})", shopFilterStr);
                    }
                    if (statusFilter != "ALL") {
                        selectQuery += std::format(" AND inv.payment_status = '{}'", statusFilter);
                    }
                    selectQuery += std::format(" ORDER BY inv.invoice_id DESC LIMIT {} OFFSET {}", itemsPerPage, offset);

                    auto selectRes = db.executeQuery(selectQuery);
                    if (selectRes) {
                        sql::ResultSet* rs = selectRes.value();
                        while (rs->next()) {
                            double base = rs->getDouble("base_fee");
                            double dep = rs->getDouble("security_deposit");
                            double late = rs->getDouble("late_fee");
                            double tot = rs->getDouble("total_amount");
                            std::string status = rs->getString("payment_status");

                            tool::ui::printRow(colWidths, {
                                rs->getString("inv_uid"),
                                rs->getString("rnt_uid"),
                                rs->getString("customer_name"),
                                format("RM {:.2f}", base),
                                format("RM {:.2f}", dep),
                                format("RM {:.2f}", late),
                                format("RM {:.2f}", tot),
                                status
                            });
                        }
                        delete rs;
                    } else {
                        println("  Error fetching invoice ledger: {}", selectRes.error());
                    }

                    tool::helper::drawLine(118, '=');

                    // Pagination details
                    tool::ui::printPaginationFooter(currentPage, totalPages, totalItems, 118);
                    println("  [F] Filter Status  [0] Back to Reports Gateway");
                    tool::helper::drawLine(118, '-');
                    print("  Enter selection: ");

                    string ledgerInput;
                    getline(cin, ledgerInput);

                    if (ledgerInput == "0") {
                        invalidAttemptsLedger = 0;
                        inLedger = false;
                    } else if (ledgerInput == "N" || ledgerInput == "n") {
                        invalidAttemptsLedger = 0;
                        if (currentPage < totalPages) currentPage++;
                    } else if (ledgerInput == "P" || ledgerInput == "p") {
                        invalidAttemptsLedger = 0;
                        if (currentPage > 1) currentPage--;
                    } else if (ledgerInput == "F" || ledgerInput == "f") {
                        invalidAttemptsLedger = 0;
                        println("\n  Select Status Filter:");
                        println("  [1] All Invoices");
                        println("  [2] Paid (Active Rentals)");
                        println("  [3] Settled (Completed Returns)");
                        println("  [4] Overdue (Outstanding Surcharges)");
                        print("  Enter choice: ");
                        string fChoice;
                        getline(cin, fChoice);
                        if (fChoice == "1") statusFilter = "ALL";
                        else if (fChoice == "2") statusFilter = "Paid";
                        else if (fChoice == "3") statusFilter = "Settled";
                        else if (fChoice == "4") statusFilter = "Overdue";
                        currentPage = 1;
                    } else {
                        println("  Invalid choice.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttemptsLedger)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                    }
                }
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
