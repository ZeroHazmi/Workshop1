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

    void showMonthlyRevenueReport(
        vector<int>& activeShopIds, 
        vector<string>& activeShopNames,
        stats::DateRange& activeDateRange, 
        string& activeDateRangeLabel
    ) {
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

            auto trendsOpt = stats::getRevenueTrends(activeShopIds, activeDateRange);
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
                    const vector<string> barColors = {
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
                        print("  {:9.2f} | ", threshold);
                        size_t cIdx = 0;
                        for (const auto& pt : trends) {
                            string color = barColors[cIdx % barColors.size()];
                            cIdx++;
                            if (pt.revenue >= threshold && pt.revenue > 0.0) {
                                print("   {}{}{}   ", color, "███", "\033[0m");
                            } else {
                                print("         ");
                            }
                        }
                        print("\n");
                    }

                    // Chart base line
                    print("            +");
                    for (size_t i = 0; i < trends.size(); ++i) {
                        print("---------");
                    }
                    print("\n             ");

                    // X-axis month labels formatted to exactly 6 characters (e.g., "May 26") and centered
                    for (const auto& pt : trends) {
                        string displayMonth = pt.month_name;
                        if (displayMonth.length() == 8 && displayMonth[3] == ' ') {
                            displayMonth = displayMonth.substr(0, 3) + " " + displayMonth.substr(6, 2);
                        }
                        print("  {:^6} ", displayMonth);
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
    }
}