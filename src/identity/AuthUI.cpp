#include "identity/Auth/Auth.h"
#include "identity/AdminUI.h"
#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/InventoryUI.h"
#include "identity/Profile/Profile.h"
#include "transaction/Rental/Rental.h"
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
        while (inDashboard) {
            showCustomerDashboard(session);
            
            int subChoice;
            if (!(cin >> subChoice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');  // Clear the input buffer after reading choice

            switch (subChoice) {
                case 1: 
                    // inventory::browseApparel
                    inventory::ui::showCatalog(session);
                    break;
                case 2: 
                    handleRentalHistoryMenu(session); 
                    break;
                case 3: 
                    // profile::updateProfile(session.userid);
                    viewProfile(session); 
                    break;
                case 4: 
                    // profile::manageBank(session.userid);
                    manageBankAccount(session);
                    break;
                case 0:
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
                    println("Invalid option.");
            }
        }
    }

    void viewProfile(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("USER PROFILE", 64);
        tool::helper::drawLine(64, '=');
        println(""); // Spacer

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
        
        println("");
        tool::helper::drawLine(64, '-');
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void manageBankAccount(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("BANK ACCOUNT DETAILS", 64);
        tool::helper::drawLine(64, '=');
        
        println(""); // Spacer

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
            if (cin >> option) {
                if (option == 1) {
                    cin.ignore(1000, '\n');
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
                    string waitInput;
                    do {
                        print("\nEnter '0' to return to dashboard: ");
                        getline(cin, waitInput);
                    } while (waitInput != "0");
                    return;
                } else if (option == 2) {
                    cin.ignore(1000, '\n');
                    auto removeRes = ::identity::profile::Profile::removeBankAccount(bank.acc_id);
                    if (removeRes) {
                        println("\n  Bank account successfully unlinked.");
                    } else {
                        println("\n  Failed to unlink bank account: {}", removeRes.error());
                    }
                    string waitInput;
                    do {
                        print("\nEnter '0' to return to dashboard: ");
                        getline(cin, waitInput);
                    } while (waitInput != "0");
                    return;
                } else {
                    cin.ignore(1000, '\n');
                    return;
                }
            } else {
                cin.clear();
                cin.ignore(1000, '\n');
                return;
            }
        } else {
            println("  No bank account linked to your profile yet.");
            println("\n  [1] Link a Bank Account");
            println("  [0] Back to Dashboard");
            print("\n  Enter selection: ");
            
            int choice;
            if (cin >> choice && choice == 1) {
                cin.ignore(1000, '\n');
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
                string waitInput;
                do {
                    print("\nEnter '0' to return to dashboard: ");
                    getline(cin, waitInput);
                } while (waitInput != "0");
                return;
            } else {
                cin.clear();
                cin.ignore(1000, '\n');
                return;
            }
        }
    }

    void viewRentalHistory(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("RENTAL HISTORY", 75);
        println("");

        auto historyResult = ::transaction::rental::getCustomerRentalHistory(session.userid);
        if (!historyResult) {
            println("  Error: {}", historyResult.error());
            string waitInput;
            do {
                print("\nEnter '0' to return to dashboard: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        auto& history = historyResult.value();
        if (history.empty()) {
            println("  You have no active or past rentals in your history.");
            println("");
            tool::helper::drawLine(75, '-');
            string waitInput;
            do {
                print("\nEnter '0' to return to dashboard: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        // Print header for the table
        std::vector<int> colWidths = {12, 22, 10, 11, 10, 10};
        tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "RENT DATE", "RET DATE", "PAID FEE", "STATUS"});
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

        tool::helper::drawLine(75, '=');
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void handleRentalHistoryMenu(const ::identity::auth::UserSession& session) {
        bool inSubMenu = true;
        while (inSubMenu) {
            tool::helper::clearScreen();
            tool::helper::drawLine(50, '=');
            tool::ui::displayTitle("RENTAL HISTORY GATEWAY", 50);
            tool::helper::drawLine(50, '=');
            println("");
            println("  [1] View Detailed Transaction Log");
            println("  [2] View Booking Behaviour & Insights (Graphs)");
            println("  [0] Back to Main Menu");
            println("");
            tool::helper::drawLine(50, '-');
            print("  Select option: ");

            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');

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
        }
    }

    void viewBookingBehaviour(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("BOOKING BEHAVIOUR & INSIGHTS", 60);
        println("");

        auto statsRes = ::transaction::rental::getCustomerBookingStats(session.userid);
        if (!statsRes) {
            println("  Error retrieving statistics: {}", statsRes.error());
            string waitInput;
            do {
                print("\nEnter '0' to return to previous menu: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
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
            println("");
            tool::helper::drawLine(60, '-');
            string waitInput;
            do {
                print("\nEnter '0' to return to previous menu: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        // 1. POPULAR CATEGORIES (Horizontal Bar Chart)
        println("  [1] CATEGORY POPULARITY");
        println("  --------------------------------------------------------");
        int maxCount = stats.categories.empty() ? 0 : stats.categories[0].count;
        int maxBarWidth = 25;
        for (const auto& cat : stats.categories) {
            int percentage = (cat.count * 100) / totalRentals;
            int barWidth = maxCount > 0 ? (cat.count * maxBarWidth) / maxCount : 0;
            std::string bar = "";
            for (int i = 0; i < barWidth; ++i) {
                bar += "█";
            }
            std::string spaces = std::string(maxBarWidth - barWidth, ' ');
            println("  {:<20} : [ {}{} ] {}% ({} rentals)", cat.category, bar, spaces, percentage, cat.count);
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
            
            println("  Split Ratio: [ {}{} ]", onTimeStr, lateStr);
            println("  Legend     : (█) On-Time [{} | {}%]  (░) Overdue [{} | {}%]", 
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
            int maxHeight = 8;
            for (int h = maxHeight; h >= 1; --h) {
                int threshold = (maxMonthCount * h) / maxHeight;
                std::print("  {:3d} | ", threshold);
                for (const auto& m : stats.monthly_trends) {
                    if (m.count >= threshold && m.count > 0) {
                        std::print("   ███   ");
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

        println("");
        tool::helper::drawLine(60, '=');
        string waitInput;
        do {
            print("\nEnter '0' to return to previous menu: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void showAdminDashboard(const ::identity::auth::UserSession& session) {
        // Delegate to admin UI
        ::identity::adminui::showAdminDashboard(session);
    }

    void handleAdminDashboard(const ::identity::auth::UserSession& session) {
        // Delegate to admin UI
        ::identity::adminui::handleAdminDashboard(session);
    }

}