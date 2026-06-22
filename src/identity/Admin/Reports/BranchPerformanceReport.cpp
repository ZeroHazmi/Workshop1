#include "identity/Admin/Reports/ReportsInternal.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/Colors.h"
#include <print>
#include <string>
#include <iostream>
#include <format>
#include <vector>

namespace stats = ::transaction::rental::stats;

using namespace std;

namespace identity::adminui::reports {

    void showBranchPerformanceReport(
        vector<int>& activeShopIds, 
        vector<string>& activeShopNames,
        stats::DateRange& activeDateRange, 
        string& activeDateRangeLabel
    ) {
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

            auto branchOpt = stats::getBranchPerformance(activeShopIds, activeDateRange);
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
                        const auto& barColors = tool::colors::GRAPH_PALETTE;
                        size_t cIdx = 0;
                        for (const auto& pt : branches) {
                            double pct = (pt.revenue / totalRevenue) * 100.0;
                            int barChars = (int)(pct / 2.5); // 40 chars total
                            string barStr = "";
                            for (int i = 0; i < barChars; ++i) barStr += "█";
                            string remStr = "";
                            for (int i = 0; i < (40 - barChars); ++i) remStr += "░";
                            
                            string_view color = barColors[cIdx % barColors.size()];
                            cIdx++;
                            
                            print("  {:<20} [ {}{}{}{} ] {:5.1f}% (RM {:.2f})\n", 
                                pt.shop_name, color, barStr, tool::colors::RESET, remStr, pct, pt.revenue);
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
    }
}