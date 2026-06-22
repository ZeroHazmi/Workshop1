#include "identity/AuthUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/InventoryUI.h"
#include "tool/input.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>

namespace auth = ::identity::auth;

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
        println("\n");
    }

    void showCustomerDashboard(const auth::UserSession& session) {
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
        println("  [3] View Personal Profile");
        println("  [4] Manage Bank Account Details");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleCustomerDashboard(const auth::UserSession& session) {
        bool inDashboard = true;
        while (inDashboard) {
            showCustomerDashboard(session);
            
            int subChoice;
            if (!tool::input::readInt(subChoice)) {
                println("  Invalid selection.");
                this_thread::sleep_for(chrono::milliseconds(1000));
                continue;
            }

            switch (subChoice) {
                case 1: 
                    inventory::ui::showCatalog(session);
                    break;
                case 2: 
                    handleRentalHistoryMenu(session); 
                    break;
                case 3: 
                    viewProfile(session); 
                    break;
                case 4: 
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
                    println("  Invalid selection.");
                    this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
    }

}