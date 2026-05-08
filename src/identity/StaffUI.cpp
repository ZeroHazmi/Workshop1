#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::staffui {
    
    void showStaffDashboard(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        
        // Header with double-line borders
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("STAFF DASHBOARD", 64);
        tool::helper::drawLine(64, '=');

        // User Context
        println("\n  Welcome Staff, {}!", session.username);
        println("  Role: {}\n", session.roles.front());
        
        // Staff Menu Options
        println("  [1] Register/Add New Apparel");
        println("  [2] View Rental Apparel List");
        println("  [3] Modify Rental Details");
        println("  [4] View Staff Profile");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleStaffDashboard(const ::identity::auth::UserSession& session) {
        bool inStaffPanel = true;
        while (inStaffPanel) {
            showStaffDashboard(session);
            
            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');  // Clear input buffer

            switch (choice) {
                case 1:
                    registerNewApparel();
                    break;
                case 2:
                    viewRentalApparelList();
                    break;
                case 3:
                    modifyRentalDetails();
                    break;
                case 4:
                    viewStaffProfile(session);
                    break;
                case 0:
                    println("\nLogging out...");
                    inStaffPanel = false;

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

    void registerNewApparel() {
        tool::ui::displayTitle("REGISTER NEW APPAREL", 50);
        println("");

        string apparelName, category, condition;
        double rentalPrice;
        int quantity;

        cin.ignore(1000, '\n');
        print("  Apparel Name: ");
        getline(cin, apparelName);
        
        print("  Category: ");
        getline(cin, category);
        
        print("  Rental Price (per day): RM ");
        cin >> rentalPrice;
        
        print("  Quantity: ");
        cin >> quantity;
        
        cin.ignore(1000, '\n');
        print("  Condition: ");
        getline(cin, condition);

        // TODO: Call database function to insert apparel
        println("\n  Apparel registered successfully!");
        
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void viewRentalApparelList() {
        tool::ui::displayTitle("RENTAL APPAREL LIST", 80);
        println("");

        vector<int> colWidths = {4, 20, 12, 12, 15, 10};
        tool::ui::printRow(colWidths, {"ID", "APPAREL NAME", "CUSTOMER", "DEPOSIT PAID", "RETURN DATE", "STATUS"});
        tool::helper::drawLine(80, '-');

        // TODO: Fetch from database with rental info and deposit status
        tool::ui::printRow(colWidths, {"001", "Baju Melayu", "Ahmad", "YES", "2026-05-05", "Active"});
        tool::ui::printRow(colWidths, {"002", "Tuxedo", "Fatimah", "NO", "2026-05-10", "Pending"});

        tool::helper::drawLine(80, '=');
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void modifyRentalDetails() {
        tool::ui::displayTitle("MODIFY RENTAL DETAILS", 50);
        println("");

        int rentalId;
        print("  Enter Rental ID to modify: ");
        cin >> rentalId;
        cin.ignore(1000, '\n');

        // Display options
        println("\n  [1] Set Return Date");
        println("  [2] Set Condition");
        println("  [3] Set Status");
        println("  [4] Back to Dashboard");
        print("  Select option: ");

        int option;
        cin >> option;
        cin.ignore(1000, '\n');

        switch (option) {
            case 1: {
                print("  Enter new return date (YYYY-MM-DD): ");
                string returnDate;
                getline(cin, returnDate);
                // TODO: Update in database
                println("  Return date updated!");
                break;
            }
            case 2: {
                print("  Enter condition (Excellent/Good/Fair/Poor): ");
                string condition;
                getline(cin, condition);
                // TODO: Update in database
                println("  Condition updated!");
                break;
            }
            case 3: {
                println("  Status options:");
                println("  [1] Active");
                println("  [2] Returned");
                println("  [3] Damaged");
                print("  Select status: ");
                int statusChoice;
                cin >> statusChoice;
                cin.ignore(1000, '\n');
                // TODO: Update in database
                println("  Status updated!");
                break;
            }
            case 4:
                break;
            default:
                println("  Invalid option.");
        }

        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void viewStaffProfile(const ::identity::auth::UserSession& session) {
        tool::ui::displayTitle("STAFF PROFILE", 50);
        println("");

        // TODO: Fetch actual staff data from database
        tool::ui::printField("Username", session.username);
        tool::ui::printField("Role", session.roles.front());
        tool::ui::printField("Staff Email", "staff@utem.edu.my");
        tool::ui::printField("Department", "Rental Management");
        tool::ui::printField("Assigned Shop", "Main Shop");
        
        println("");
        println("  (Note: Contact admin to modify profile information)");
        tool::helper::drawLine(50, '-');
        
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }
}
