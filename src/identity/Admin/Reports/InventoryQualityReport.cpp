#include "identity/Admin/Reports/ReportsInternal.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include <print>
#include <string>
#include <iostream>
#include <format>
#include <vector>

namespace stats = ::transaction::rental::stats;

using namespace std;

namespace identity::adminui::reports {

    void showInventoryQualityReport(
        vector<int>& activeShopIds, 
        vector<string>& activeShopNames
    ) {
        bool inReport = true;
        while (inReport) {
            tool::ui::showHeader("INVENTORY QUALITY AUDIT", 64);
            
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
            print(" | Time Range: N/A (Current Physical State)\n");
            println("");

            auto auditOpt = stats::getInventoryConditionAudit(activeShopIds);
            if (!auditOpt) {
                print("{}\n", format("  Error retrieving inventory audit: {}", auditOpt.error()));
            } else {
                auto conditions = auditOpt.value();
                
                // Predefine all standard condition statuses in logical order
                vector<string> allStatuses = { "Excellent", "Good", "Fair", "Poor", "Damaged" };
                vector<pair<string, int>> finalConditions;
                
                // Populate standard statuses
                for (const auto& status : allStatuses) {
                    int count = 0;
                    for (const auto& pt : conditions) {
                        if (pt.condition == status) {
                            count = pt.count;
                            break;
                        }
                    }
                    finalConditions.push_back({status, count});
                }
                
                // Add any other unexpected statuses from DB
                for (const auto& pt : conditions) {
                    bool found = false;
                    for (const auto& status : allStatuses) {
                        if (pt.condition == status) {
                            found = true;
                            break;
                        }
                    }
                    if (!found) {
                        finalConditions.push_back({pt.condition, pt.count});
                    }
                }

                int totalItems = 0;
                for (const auto& pair : finalConditions) {
                    totalItems += pair.second;
                }

                vector<int> colWidths = {25, 15, 15};
                tool::helper::drawLine(64, '-');
                tool::ui::printRow(colWidths, {"CONDITION STATUS", "ITEM COUNT", "PERCENTAGE"});
                tool::helper::drawLine(64, '-');

                for (const auto& pair : finalConditions) {
                    double pct = totalItems > 0 ? (pair.second / (double)totalItems) * 100.0 : 0.0;
                    string countStr = to_string(pair.second);
                    string pctStr = format("{:.1f}%", pct);
                    tool::ui::printRow(colWidths, {pair.first, countStr, pctStr});
                }
                tool::helper::drawLine(64, '-');
                string totalItemsStr = to_string(totalItems);
                tool::ui::printRow(colWidths, {"TOTAL ITEMS", totalItemsStr, "100.0%"});
                tool::helper::drawLine(64, '=');
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
    }
}