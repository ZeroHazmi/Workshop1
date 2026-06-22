#include "identity/AuthUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include "tool/Colors.h"
#include "transaction/Rental/Rental.h"
#include <print>
#include <string>
#include <format>
#include <iostream>
#include <vector>
#include <thread>

namespace auth = ::identity::auth;
namespace rental = ::transaction::rental;

using namespace std;

namespace identity::authui {

    void viewRentalHistory(const auth::UserSession& session) {
        tool::ui::showHeader("RENTAL HISTORY", 75);

        auto historyResult = rental::getCustomerRentalHistory(session.userid);
        if (!historyResult) {
            println("  Error: {}", historyResult.error());
            tool::ui::pressZeroToReturn("dashboard", 75);
            return;
        }

        auto& history = historyResult.value();
        if (history.empty()) {
            println("  You have no active or past rentals in your history.");
            tool::ui::pressZeroToReturn("dashboard", 75);
            return;
        }

        // Print header for the table
        vector<int> colWidths = {12, 20, 11, 12, 10, 10};
        tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "RENTAL DATE", "RETURN DATE", "PAID FEE", "STATUS"});
        tool::helper::drawLine(75, '-');

        for (const auto& item : history) {
            double totalPaid = item.base_fee + item.late_fee;
            string actualRet = item.actual_return_date;
            
            tool::ui::printRow(colWidths, {
                item.unique_id,
                item.item_name,
                item.rental_date,
                actualRet,
                format("RM {:.2f}", totalPaid),
                item.payment_status
            });
        }

        tool::ui::pressZeroToReturn("dashboard", 75);
    }

    void handleRentalHistoryMenu(const auth::UserSession& session) {
        bool inSubMenu = true;
        while (inSubMenu) {
            tool::ui::showHeader("RENTAL HISTORY GATEWAY", 50);
            println("  [1] View Detailed Transaction Log");
            println("  [2] View Booking Behaviour & Insights (Graphs)");
            println("  [0] Back to Main Menu");
            println("");
            tool::helper::drawLine(50, '-');
            print("  Select option: ");

            int choice;
            if (tool::input::readInt(choice)) {
                switch (choice) {
                    case 1:
                        viewRentalHistory(session);
                        break;
                    case 2:
                        viewBookingBehaviour(session);
                        break;
                    case 0:
                        inSubMenu = false;
                        break;
                    default:
                        println("  Invalid selection.");
                        this_thread::sleep_for(chrono::milliseconds(1000));
                }
            } else {
                println("  Invalid selection.");
                this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
    }

    void viewBookingBehaviour(const auth::UserSession& session) {
        tool::ui::showHeader("BOOKING BEHAVIOUR & INSIGHTS", 60);

        auto statsRes = rental::getCustomerBookingStats(session.userid);
        if (!statsRes) {
            println("  Error retrieving statistics: {}", statsRes.error());
            tool::ui::pressZeroToReturn("previous menu", 60);
            return;
        }

        auto& stats = statsRes.value();
        
        // Calculate totals
        int totalRentals = 0;
        for (const auto& cat : stats.categories) {
            totalRentals += cat.count;
        }

        if (totalRentals == 0) {
            println("  No rental history found. Book some attire first to unlock insights!");
            tool::ui::pressZeroToReturn("previous menu", 60);
            return;
        }

        // Color palette for graphs (12 unique distinct colors)
        const auto& barColors = tool::colors::GRAPH_PALETTE;

        // 1. POPULAR CATEGORIES (Horizontal Bar Chart)
        println("  [1] CATEGORY POPULARITY");
        println("  --------------------------------------------------------");
        int maxCount = stats.categories.empty() ? 0 : stats.categories[0].count;
        int maxBarWidth = 25;

        size_t maxCategoryLen = 20; // default minimum width
        for (const auto& cat : stats.categories) {
            if (cat.category.length() > maxCategoryLen) {
                maxCategoryLen = cat.category.length();
            }
        }

        size_t catColorIdx = 0;
        for (const auto& cat : stats.categories) {
            int percentage = (cat.count * 100) / totalRentals;
            int barWidth = maxCount > 0 ? (cat.count * maxBarWidth) / maxCount : 0;
            string bar = "";
            for (int i = 0; i < barWidth; ++i) {
                bar += "█";
            }
            string spaces = string(maxBarWidth - barWidth, ' ');
            string_view color = barColors[catColorIdx % barColors.size()];
            catColorIdx++;

            print("  {:<{}} : [ {}{}{} ] {}% ({} rentals)\n", 
                       cat.category, maxCategoryLen, color, bar, string(tool::colors::RESET) + spaces, percentage, cat.count);
        }
        println("");

        // 2. RETURN DISCIPLINE (Ratio Split)
        println("  [2] RETURN DISCIPLINE RATIO");
        println("  --------------------------------------------------------");
        auto& r = stats.return_behaviour;
        int onTimeTotal = r.on_time + r.active_on_time;
        int lateTotal = r.late + r.active_overdue;
        int sumReturn = onTimeTotal + lateTotal;

        if (sumReturn > 0) {
            int onTimeBar = (onTimeTotal * 40) / sumReturn; // 40 chars total
            int lateBar = 40 - onTimeBar;
            string onTimeStr = "";
            for (int i = 0; i < onTimeBar; ++i) onTimeStr += "█";
            string lateStr = "";
            for (int i = 0; i < lateBar; ++i) lateStr += "░";
            
            print("  Split Ratio: [ {}{}{}{} ]\n", tool::colors::BRIGHT_GREEN, onTimeStr, tool::colors::BRIGHT_RED, lateStr, tool::colors::RESET);
            print("  Legend     : ({}) On-Time [{} | {}%]  ({}) Overdue [{} | {}%]\n", 
                    format("{}{}{}", tool::colors::BRIGHT_GREEN, "█", tool::colors::RESET),
                    onTimeTotal, (onTimeTotal * 100) / sumReturn,
                    format("{}{}{}", tool::colors::BRIGHT_RED, "░", tool::colors::RESET),
                    lateTotal, (lateTotal * 100) / sumReturn);
        } else {
            println("  No return tracking logs available.");
        }
        println("");

        // 3. BOOKING DENSITY TRENDS (Vertical Histogram Chart)
        println("  [3] BOOKING ACTIVITY TRENDS (Last 6 Months)");
        println("  --------------------------------------------------------");
        
        int maxMonthCount = 0;
        for (const auto& m : stats.monthly_trends) {
            if (m.count > maxMonthCount) maxMonthCount = m.count;
        }

        if (maxMonthCount > 0) {
            println("  Total (Rentals)");
            int maxHeight = 8;
            for (int h = maxHeight; h >= 1; --h) {
                int threshold = (maxMonthCount * h) / maxHeight;
                print("  {:3d} | ", threshold);
                size_t mIdx = 0;
                for (const auto& m : stats.monthly_trends) {
                    string_view color = barColors[mIdx % barColors.size()];
                    mIdx++;
                    if (m.count >= threshold && m.count > 0) {
                        print("   {}{}{}   ", color, "███", tool::colors::RESET);
                    } else {
                        print("         ");
                    }
                }
                print("\n");
            }
            
            // X-Axis base line (perfectly aligned directly above month names with no vertical white spaces)
            print("      +");
            for (size_t i = 0; i < stats.monthly_trends.size(); ++i) {
                print("---------");
            }
            print("\n       ");

            // Month Labels (centered and formatted to 6 characters: Month Yr)
            for (const auto& m : stats.monthly_trends) {
                string displayMonth = m.month_name;
                if (displayMonth.length() == 8 && displayMonth[3] == ' ') {
                    displayMonth = displayMonth.substr(0, 3) + " " + displayMonth.substr(6, 2);
                } else if (displayMonth.length() > 6) {
                    displayMonth = displayMonth.substr(0, 6);
                }
                print("  {:^6} ", displayMonth);
            }
            print("\n\n");

            // Print X-axis label centered
            int xLabelWidth = 9 * static_cast<int>(stats.monthly_trends.size());
            int xLabelPad = 7 + (xLabelWidth - 6) / 2;
            if (xLabelPad < 0) xLabelPad = 0;
            println("{}{}", string(xLabelPad, ' '), "Months");
            println("");

            // Tabular details for clean precise reference
            tool::helper::drawLine(60, '-');
            print("{}\n", format("  {:<20} | {:<20}", "MONTH", "RENTALS COUNT"));
            tool::helper::drawLine(60, '-');
            for (const auto& m : stats.monthly_trends) {
                print("{}\n", format("  {:<20} | {:<18d}", m.month_name, m.count));
            }
            tool::helper::drawLine(60, '=');
        } else {
            println("  No monthly active records available.");
        }

        tool::ui::pressZeroToReturn("previous menu", 60);
    }

}