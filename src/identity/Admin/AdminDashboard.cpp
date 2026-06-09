#include "identity/AdminUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>
#include <format>

namespace auth = ::identity::auth;

using namespace std;

namespace identity::adminui {

    void showAdminDashboard(const auth::UserSession& session) {
        tool::helper::clearScreen();
        
        // Header with double-line borders
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("ADMIN DASHBOARD", 64);
        tool::helper::drawLine(64, '=');

        // User Context
        print("{}\n", format("  Welcome {}!", session.username));
        print("{}\n\n", format("  Role: {}", session.roles.front()));
        
        // Admin Menu Options
        println("  [1] Staff Management");
        println("  [2] Shop & Inventory Management");
        println("  [3] Business Reports & Statistics");
        println("  [4] View Admin Profile");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleAdminDashboard(const auth::UserSession& session) {
        bool inAdminPanel = true;
        int invalidAttempts = 0;
        while (inAdminPanel) {
            showAdminDashboard(session);
            
            int choice;
            if (!tool::input::readInt(choice)) {
                println("  Invalid selection.");
                if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
                continue;
            }

            switch (choice) {
                case 1:
                    invalidAttempts = 0;
                    showStaffManagementSubmenu(session);
                    break;
                case 2:
                    invalidAttempts = 0;
                    showShopManagementSubmenu(session);
                    break;
                case 3:
                    invalidAttempts = 0;
                    showBusinessStatsSubmenu(session);
                    break;
                case 4:
                    invalidAttempts = 0;
                    viewAdminProfile(session);
                    break;
                case 0:
                    invalidAttempts = 0;
                    println("\nLogging out...");
                    inAdminPanel = false;

                    for (int i = 0; i < 3; ++i) {
                        this_thread::sleep_for(chrono::milliseconds(700));
                        print(".");
                    }
                    println("\n");
                    tool::helper::clearScreen();
                    break;
                default:
                    println("  Invalid option.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
            }
        }
    }

    void viewAdminProfile(const auth::UserSession& session) {
        tool::ui::showHeader("ADMIN PROFILE", 64);

        tool::ui::printField("Username", session.username);
        tool::ui::printField("Role", session.roles.front());
        tool::ui::printField("Admin Email", session.username + "@utem.edu.my");
        tool::ui::printField("Department", "FWCRS System Administration");
        
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

}