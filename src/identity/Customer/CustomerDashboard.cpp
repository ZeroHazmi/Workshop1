#include "identity/AuthUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/InventoryUI.h"
#include "tool/input.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::authui {

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

}
