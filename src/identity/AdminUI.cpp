#include "identity/AdminUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::adminui {
    
    void showAdminDashboard(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        
        // Header with double-line borders
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("ADMIN DASHBOARD", 64);
        tool::helper::drawLine(64, '=');

        // User Context
        println("\n  Welcome Admin, {}!", session.username);
        println("  Role: {}\n", session.roles.front());
        
        // Admin Menu Options
        println("  [1] Register Staff Account");
        println("  [2] Manage Staff Account");
        println("  [3] View Admin Profile");
        println("  [4] Register New Shop");
        println("  [5] Manage Shop Information");
        println("  [6] Display Shop List");
        println("  [7] View Shop/Branch Inventory");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleAdminDashboard(const ::identity::auth::UserSession& session) {
        bool inAdminPanel = true;
        while (inAdminPanel) {
            showAdminDashboard(session);
            
            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');  // Clear input buffer

            switch (choice) {
                case 1:
                    registerStaffAccount();
                    break;
                case 2:
                    manageStaffAccount();
                    break;
                case 3:
                    viewAdminProfile(session);
                    break;
                case 4:
                    registerNewShop();
                    break;
                case 5:
                    manageShopInformation();
                    break;
                case 6:
                    displayShopList();
                    break;
                case 7:
                    viewShopInventory();
                    break;
                case 0:
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
                    println("Invalid option.");
            }
        }
    }

    void registerStaffAccount() {
        tool::helper::clearScreen();
        tool::ui::displayTitle("REGISTER STAFF ACCOUNT", 50);
        println("");

        // Use the new registration flow with staff role
        ::identity::auth::Auth::handleRegisterFlow("staff");
        
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void manageStaffAccount() {
        tool::ui::displayTitle("MANAGE STAFF ACCOUNT", 50);
        println("");

        int staffId;
        print("  Enter Staff ID to manage: ");
        cin >> staffId;
        cin.ignore(1000, '\n');

        // Display staff information
        println("\n  [1] Update Staff Information");
        println("  [2] Change Shop Assignment");
        println("  [3] Back to Dashboard");
        print("  Select option: ");

        int option;
        cin >> option;
        cin.ignore(1000, '\n');

        switch (option) {
            case 1:
                // TODO: Update staff information
                println("  Updating staff information...");
                break;
            case 2:
                // TODO: Change shop assignment
                println("  Changing shop assignment...");
                break;
            case 3:
                break;
            default:
                println("  Invalid option.");
        }

        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void viewAdminProfile(const ::identity::auth::UserSession& session) {
        tool::ui::displayTitle("ADMIN PROFILE", 50);
        println("");

        // TODO: Fetch actual admin data from database
        tool::ui::printField("Username", session.username);
        tool::ui::printField("Role", session.roles.front());
        tool::ui::printField("Admin Email", "admin@utem.edu.my");
        tool::ui::printField("Department", "System Administration");
        
        println("");
        tool::helper::drawLine(50, '-');
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void registerNewShop() {
        tool::ui::displayTitle("REGISTER NEW SHOP", 50);
        println("");

        string shopName, location, manager;
        print("  Shop Name: ");
        cin.ignore(1000, '\n');
        getline(cin, shopName);
        
        print("  Location: ");
        getline(cin, location);
        
        print("  Manager Name: ");
        getline(cin, manager);

        // TODO: Call database function to insert shop
        println("\n  Shop registered successfully!");
        
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void manageShopInformation() {
        tool::ui::displayTitle("MANAGE SHOP INFORMATION", 50);
        println("");

        int shopId;
        print("  Enter Shop ID to manage: ");
        cin >> shopId;
        cin.ignore(1000, '\n');

        // TODO: Display shop info and allow updates
        println("  Shop information:");
        println("  [1] Update Shop Name");
        println("  [2] Update Location");
        println("  [3] Update Manager");
        println("  [4] Back to Dashboard");
        print("  Select option: ");

        int option;
        cin >> option;
        cin.ignore(1000, '\n');

        // TODO: Handle updates based on selection

        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void displayShopList() {
        tool::ui::displayTitle("SHOP LIST", 65);
        println("");

        vector<int> colWidths = {5, 25, 20, 15};
        tool::ui::printRow(colWidths, {"ID", "SHOP NAME", "LOCATION", "MANAGER"});
        tool::helper::drawLine(65, '-');

        // TODO: Fetch from database and display
        tool::ui::printRow(colWidths, {"001", "Main Shop", "Building A, UTeM", "Zal"});
        tool::ui::printRow(colWidths, {"002", "Branch Shop", "Building B, UTeM", "Ali"});

        tool::helper::drawLine(65, '=');
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void viewShopInventory() {
        tool::ui::displayTitle("SHOP/BRANCH INVENTORY", 65);
        println("");

        int shopId;
        print("  Enter Shop ID: ");
        cin >> shopId;
        cin.ignore(1000, '\n');

        vector<int> colWidths = {4, 25, 12, 10};
        tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "QUANTITY", "CONDITION"});
        tool::helper::drawLine(65, '-');

        // TODO: Fetch inventory for specified shop from database
        tool::ui::printRow(colWidths, {"001", "Baju Melayu (Satin)", "15", "Good"});
        tool::ui::printRow(colWidths, {"002", "Tuxedo Black (Slim)", "8", "Good"});

        tool::helper::drawLine(65, '=');
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }
}
