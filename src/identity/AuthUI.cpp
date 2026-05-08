#include "identity/Auth/Auth.h"
#include "identity/AdminUI.h"
#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/InventoryUI.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::authui {
    // Forward declarations
    void viewProfile(const ::identity::auth::UserSession& session);
    void manageBankAccount(const ::identity::auth::UserSession& session);

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
                    inventory::ui::showCatalog();
                    break;
                case 2: 
                    // transaction::viewHistory(session.userid); 
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
        tool::ui::displayTitle("USER PROFILE", 50);
        println(""); // Spacer

        tool::ui::printField("Full Name", "Zal Hazmi");
        tool::ui::printField("Email", "zero@utem.edu.my");
        tool::ui::printField("Phone", "+6012-3456789");
        
        println("");
        tool::helper::drawLine(50, '-');
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void manageBankAccount(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("BANK ACCOUNT DETAILS", 50);
        println(""); // Spacer

        tool::ui::printField("Account Holder", "Zal Hazmi");
        tool::ui::printField("Bank Name", "Maybank");
        tool::ui::printField("Account Number", "***** ***** ***** 5678");
        tool::ui::printField("Account Type", "Savings");
        
        println("");
        tool::helper::drawLine(50, '-');
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
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