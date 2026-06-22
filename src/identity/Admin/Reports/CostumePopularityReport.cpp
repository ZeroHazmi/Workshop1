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

    void showCostumePopularityReport(
        vector<int>& activeShopIds, 
        vector<string>& activeShopNames,
        stats::DateRange& activeDateRange, 
        string& activeDateRangeLabel
    ) {
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

            auto popOpt = stats::getCostumePopularity(activeShopIds, activeDateRange);
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
                    const auto& barColors = tool::colors::GRAPH_PALETTE;
                    size_t cIdx = 0;
                    for (const auto& pt : points) {
                        int barWidth = maxCount > 0 ? (pt.rental_count * maxBarWidth / maxCount) : 0;
                        string unicodeBar = "";
                        for (int i = 0; i < barWidth; ++i) {
                            unicodeBar += "█";
                        }
                        string spaces(maxBarWidth - barWidth, ' ');
                        string_view color = barColors[cIdx % barColors.size()];
                        cIdx++;
                        print("  {:<{}} | {}{}{} ({} rentals)\n", 
                                   pt.catalog_name, maxCatalogNameLen, color, unicodeBar + spaces, tool::colors::RESET, pt.rental_count);
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