#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/Apparel/Apparel.h"
#include "identity/Profile/Profile.h"
#include "inventory/InventoryUI.h"
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
        println("  [2] View Apparel Catalog");
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
                    registerNewApparel(session);
                    break;
                case 2:
                    inventory::ui::showCatalog(session);
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

    void registerNewApparel(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("REGISTER NEW APPAREL", 50);
        println("");

        auto staffProfileOpt = ::identity::profile::Profile::getStaffProfile(session.userid);
        if (!staffProfileOpt) {
            println("  Error: Could not retrieve staff profile.");
            println("\nPress Enter to return to dashboard...");
            cin.ignore(10000, '\n');
            return;
        }
        int shop_id = staffProfileOpt.value().shop_id;

        string apparelName, category, colour, description;
        double rentalPrice;

        print("  Apparel Name (or '0' to cancel): ");
        getline(cin, apparelName);
        if (apparelName == "0" || apparelName == "cancel") return;
        
        print("  Description: ");
        getline(cin, description);
        
        print("  Category: ");
        getline(cin, category);
        
        print("  Colour: ");
        getline(cin, colour);
        
        print("  Rental Price (per day): RM ");
        if (!(cin >> rentalPrice) || rentalPrice < 0) {
            cin.clear(); cin.ignore(1000, '\n'); return;
        }
        cin.ignore(1000, '\n');

        std::vector<inventory::apparel::ItemBatch> batches;
        bool addingSizes = true;
        
        while (addingSizes) {
            println("\n  --- ADD INVENTORY BATCH ---");
            string size, condition, addAnother;
            int quantity;
            
            print("  Size (e.g. S, M, L): ");
            getline(cin, size);
            
            print("  Quantity: ");
            if (!(cin >> quantity) || quantity < 0) {
                cin.clear(); cin.ignore(1000, '\n'); 
                println("  Invalid quantity. Batch cancelled.");
                continue;
            }
            cin.ignore(1000, '\n');
            
            print("  Condition (e.g. Excellent, Good): ");
            getline(cin, condition);
            
            batches.push_back({size, quantity, condition});
            
            print("  Do you want to add another size for this apparel? (Y/N): ");
            getline(cin, addAnother);
            
            if (addAnother != "Y" && addAnother != "y") {
                addingSizes = false;
            }
        }

        if (batches.empty()) {
            println("\n  Registration cancelled: No inventory batches added.");
            println("\nPress Enter to return to dashboard...");
            cin.ignore(10000, '\n');
            return;
        }

        inventory::apparel::ApparelCatalog newCatalog;
        newCatalog.shop_id = shop_id;
        newCatalog.name = apparelName;
        newCatalog.description = description;
        newCatalog.category = category;
        newCatalog.colour = colour;
        newCatalog.daily_rate = rentalPrice;

        auto result = inventory::apparel::addApparelCatalog(newCatalog, batches);
        if (result) {
            int totalQty = 0;
            for (const auto& b : batches) totalQty += b.quantity;
            println("\n  Apparel registered successfully with {} total items!", totalQty);
        } else {
            println("\n  Error: {}", result.error());
        }
        
        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void modifyRentalDetails() {
        tool::helper::clearScreen();
        tool::ui::displayTitle("MODIFY RENTAL DETAILS", 50);
        println("");

        int rentalId;
        print("  Enter Rental ID to modify (or 0 to cancel): ");
        if (!(cin >> rentalId) || rentalId == 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            return;
        }
        cin.ignore(1000, '\n');

        // Display options
        println("\n  [1] Set Return Date");
        println("  [2] Set Condition");
        println("  [3] Set Status");
        println("  [0] Back to Dashboard");
        print("  Select option: ");

        int option;
        if (!(cin >> option)) {
            cin.clear();
            cin.ignore(1000, '\n');
            return;
        }
        cin.ignore(1000, '\n');

        switch (option) {
            case 1: {
                print("  Enter new return date (DD/MM/YYYY): ");
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
                println("  [0] Back to Dashboard");
                print("  Select status: ");
                int statusChoice;
                if (!(cin >> statusChoice) || statusChoice == 0) {
                    cin.clear(); cin.ignore(1000, '\n'); break;
                }
                cin.ignore(1000, '\n');
                // TODO: Update in database
                println("  Status updated!");
                break;
            }
            case 0:
                return;
            default:
                println("  Invalid option.");
        }

        println("\nPress Enter to return to dashboard...");
        cin.ignore(10000, '\n');
    }

    void viewStaffProfile(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
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
