#include "identity/Auth/Auth.h"
#include "identity/AdminUI.h"
#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/InventoryUI.h"
#include "identity/Profile/Profile.h"
#include "transaction/Rental/Rental.h"
#include "tool/input.h"
#include <print>
#include <string>
#include <format>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::authui {
    void viewProfile(const ::identity::auth::UserSession& session);
    void manageBankAccount(const ::identity::auth::UserSession& session);
    void viewRentalHistory(const ::identity::auth::UserSession& session);
    void viewBookingBehaviour(const ::identity::auth::UserSession& session);
    void handleRentalHistoryMenu(const ::identity::auth::UserSession& session);

    void showSplashScreen() {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("FORMAL WEAR AND COSTUME RENTAL SYSTEM", 64);
        tool::ui::displayTitle("(FWCRS)", 64);
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("Premium attire management for students and university staff", 64);
        tool::helper::drawLine(64, '-');
        tool::ui::displayTitle("Location: Universiti Teknikal Malaysia Melaka (UTeM) ", 64);
        tool::helper::drawLine(64, '=');

        // for (int i = 0; i < 3; ++i) {
        //     this_thread::sleep_for(chrono::milliseconds(700));
        //     print(".");
        // }
        println("\n");

        // tool::helper::clearScreen();
    }

    void showCustomerDashboard(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        
        // Header with double-line borders
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("CUSTOMER DASHBOARD", 64);
        tool::helper::drawLine(64, '=');

        // User Context
        println("\n  Welcome back, {}!", session.username);
        println("  Role: {}\n", session.roles.front()); // Displays the primary role
        
        // Vertical Menu Options
        println("  [1] Browse Apparel Inventory");
        println("  [2] View My Rental History");
        println("  [3] Update Personal Profile");
        println("  [4] Manage Bank Account Details");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleCustomerDashboard(const ::identity::auth::UserSession& session) {
        bool inDashboard = true;
        int invalidAttempts = 0;
        while (inDashboard) {
            showCustomerDashboard(session);
            
            int subChoice;
            if (!tool::input::readInt(subChoice)) {
                println("  Invalid selection.");
                if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
                continue;
            }

            switch (subChoice) {
                case 1: 
                    invalidAttempts = 0;
                    // inventory::browseApparel
                    inventory::ui::showCatalog(session);
                    break;
                case 2: 
                    invalidAttempts = 0;
                    handleRentalHistoryMenu(session); 
                    break;
                case 3: 
                    invalidAttempts = 0;
                    // profile::updateProfile(session.userid);
                    viewProfile(session); 
                    break;
                case 4: 
                    invalidAttempts = 0;
                    // profile::manageBank(session.userid);
                    manageBankAccount(session);
                    break;
                case 0:
                    invalidAttempts = 0;
                    println("\nLogging out...");
                    inDashboard = false;

                    for (int i = 0; i < 3; ++i) {
                        this_thread::sleep_for(chrono::milliseconds(700));
                        print(".");
                    }
                    println("\n");

                    tool::helper::clearScreen();

                    break;
                default:
                    println("  Invalid selection.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
            }
        }
    }

    void viewProfile(const ::identity::auth::UserSession& session) {
        tool::ui::showHeader("USER PROFILE", 64);

        auto profileOpt = ::identity::profile::Profile::getCustomerProfile(session.userid);
        if (profileOpt) {
            auto& profile = profileOpt.value();
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Full Name", profile.fullname);
            tool::ui::printField("Email", profile.email);
            tool::ui::printField("Phone", profile.phone_no);
        } else {
            println("  Error: {}", profileOpt.error());
            println("  (Please ensure your customer profile has been fully set up.)");
        }
        
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

    void manageBankAccount(const ::identity::auth::UserSession& session) {
        bool inBankMenu = true;
        int invalidAttempts = 0;
        while (inBankMenu) {
            tool::ui::showHeader("BANK ACCOUNT DETAILS", 64);
            
            auto bankOpt = ::identity::profile::Profile::getBankAccount(session.userid);
            if (bankOpt) {
                auto& bank = bankOpt.value();
                tool::ui::printField("Account ID", bank.unique_id);
                tool::ui::printField("Account Holder", bank.acc_holder);
                tool::ui::printField("Bank Name", bank.bank_name);
                
                // Mask account number for security: e.g. show only last 4 digits
                std::string acc = bank.acc_number;
                if (acc.length() > 4) {
                    acc = std::string(acc.length() - 4, '*') + acc.substr(acc.length() - 4);
                }
                tool::ui::printField("Account Number", acc);
                tool::ui::printField("Balance", std::format("RM {:.2f}", bank.balance));
                
                println("");
                tool::helper::drawLine(50, '-');
                println("\n  [1] Deposit Funds / Add Balance");
                println("  [2] Remove Linked Bank Account");
                println("  [0] Back to Dashboard");
                print("\n  Enter selection: ");
                int option;
                if (tool::input::readInt(option)) {
                    if (option == 1) {
                        invalidAttempts = 0;
                        print("  Enter deposit amount: RM ");
                        double amount;
                        if (cin >> amount && amount > 0) {
                            cin.ignore(1000, '\n');
                            auto depositRes = ::identity::profile::Profile::depositBalance(bank.acc_id, amount);
                            if (depositRes) {
                                println("\n  Successfully deposited RM {:.2f} to your account!", amount);
                            } else {
                                println("\n  Deposit failed: {}", depositRes.error());
                            }
                        } else {
                            cin.clear();
                            cin.ignore(1000, '\n');
                            println("\n  Invalid deposit amount.");
                        }
                        tool::ui::pressZeroToReturn("bank menu", 64);
                    } else if (option == 2) {
                        invalidAttempts = 0;
                        auto removeRes = ::identity::profile::Profile::removeBankAccount(bank.acc_id);
                        if (removeRes) {
                            println("\n  Bank account successfully unlinked.");
                        } else {
                            println("\n  Failed to unlink bank account: {}", removeRes.error());
                        }
                        tool::ui::pressZeroToReturn("bank menu", 64);
                    } else if (option == 0) {
                        invalidAttempts = 0;
                        inBankMenu = false;
                    } else {
                        println("  Invalid selection.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                    }
                } else {
                    println("  Invalid selection.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
                }
            } else {
                println("  No bank account linked to your profile yet.");
                println("\n  [1] Link a Bank Account");
                println("  [0] Back to Dashboard");
                print("\n  Enter selection: ");
                
                int choice;
                if (tool::input::readInt(choice)) {
                    if (choice == 1) {
                        invalidAttempts = 0;
                        tool::helper::clearScreen();
                        tool::ui::displayTitle("LINK BANK ACCOUNT", 50);
                        println("");
                        
                        std::string bankName, accNum, holder;
                        print("  Enter Bank Name: ");
                        getline(cin, bankName);
                        print("  Enter Account Number: ");
                        getline(cin, accNum);
                        print("  Enter Account Holder Name: ");
                        getline(cin, holder);
                        
                        auto linkResult = ::identity::profile::Profile::linkBankAccount(session.userid, bankName, accNum, holder);
                        if (linkResult) {
                            println("\n  Bank account linked successfully!");
                        } else {
                            println("\n  Failed to link bank account: {}", linkResult.error());
                        }
                        tool::ui::pressZeroToReturn("bank menu", 64);
                    } else if (choice == 0) {
                        invalidAttempts = 0;
                        inBankMenu = false;
                    } else {
                        println("  Invalid selection.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                    }
                } else {
                    println("  Invalid selection.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
                }
            }
        }
    }

    void viewRentalHistory(const ::identity::auth::UserSession& session) {
        tool::ui::showHeader("RENTAL HISTORY", 75);

        auto historyResult = ::transaction::rental::getCustomerRentalHistory(session.userid);
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
        std::vector<int> colWidths = {12, 20, 11, 12, 10, 10};
        tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "RENTAL DATE", "RETURN DATE", "PAID FEE", "STATUS"});
        tool::helper::drawLine(75, '-');

        for (const auto& item : history) {
            double totalPaid = item.base_fee + item.late_fee;
            std::string actualRet = item.actual_return_date;
            
            tool::ui::printRow(colWidths, {
                item.unique_id,
                item.item_name,
                item.rental_date,
                actualRet,
                std::format("RM {:.2f}", totalPaid),
                item.payment_status
            });
        }

        tool::ui::pressZeroToReturn("dashboard", 75);
    }

    void handleRentalHistoryMenu(const ::identity::auth::UserSession& session) {
        bool inSubMenu = true;
        int invalidAttempts = 0;
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
                        invalidAttempts = 0;
                        viewRentalHistory(session);
                        break;
                    case 2:
                        invalidAttempts = 0;
                        viewBookingBehaviour(session);
                        break;
                    case 0:
                        invalidAttempts = 0;
                        inSubMenu = false;
                        break;
                    default:
                        println("  Invalid selection.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                }
            } else {
                println("  Invalid selection.");
                if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            }
        }
    }

    void viewBookingBehaviour(const ::identity::auth::UserSession& session) {
        tool::ui::showHeader("BOOKING BEHAVIOUR & INSIGHTS", 60);

        auto statsRes = ::transaction::rental::getCustomerBookingStats(session.userid);
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
            std::string bar = "";
            for (int i = 0; i < barWidth; ++i) {
                bar += "█";
            }
            std::string spaces = std::string(maxBarWidth - barWidth, ' ');
            std::string color = barColors[catColorIdx % barColors.size()];
            catColorIdx++;

            std::print("  {:<{}} : [ {}{}{} ] {}% ({} rentals)\n", 
                       cat.category, maxCategoryLen, color, bar, "\033[0m" + spaces, percentage, cat.count);
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
            std::string onTimeStr = "";
            for (int i = 0; i < onTimeBar; ++i) onTimeStr += "█";
            std::string lateStr = "";
            for (int i = 0; i < lateBar; ++i) lateStr += "░";
            
            print("  Split Ratio: [ \033[92m{}\033[91m{}\033[0m ]\n", onTimeStr, lateStr);
            print("  Legend     : (\033[92m█\033[0m) On-Time [{} | {}%]  (\033[91m░\033[0m) Overdue [{} | {}%]\n", 
                    onTimeTotal, (onTimeTotal * 100) / sumReturn,
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
                std::print("  {:3d} | ", threshold);
                size_t mIdx = 0;
                for (const auto& m : stats.monthly_trends) {
                    string color = barColors[mIdx % barColors.size()];
                    mIdx++;
                    if (m.count >= threshold && m.count > 0) {
                        std::print("   {}{}{}   ", color, "███", "\033[0m");
                    } else {
                        std::print("         ");
                    }
                }
                std::print("\n");
            }
            
            // X-Axis base line (perfectly aligned directly above month names with no vertical white spaces)
            std::print("      +");
            for (size_t i = 0; i < stats.monthly_trends.size(); ++i) {
                std::print("---------");
            }
            std::print("\n       ");

            // Month Labels (centered and formatted to 6 characters: Month Yr)
            for (const auto& m : stats.monthly_trends) {
                std::string displayMonth = m.month_name;
                if (displayMonth.length() == 8 && displayMonth[3] == ' ') {
                    displayMonth = displayMonth.substr(0, 3) + " " + displayMonth.substr(6, 2);
                } else if (displayMonth.length() > 6) {
                    displayMonth = displayMonth.substr(0, 6);
                }
                std::print("  {:^6} ", displayMonth);
            }
            std::print("\n\n");

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