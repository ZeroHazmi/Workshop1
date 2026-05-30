#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/Apparel/Apparel.h"
#include "identity/Profile/Profile.h"
#include "inventory/InventoryUI.h"
#include "tool/DateHelper.h"
#include "transaction/Rental/Rental.h"
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
        tool::ui::displayTitle("PROCESS COSTUME RETURN", 50);
        println("");

        int rentalId;
        print("  Enter Rental ID to process return (or 0 to cancel): ");
        if (!(cin >> rentalId) || rentalId == 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            return;
        }
        cin.ignore(1000, '\n');

        // Prompt for actual return date with format validation
        string returnDate;
        while (true) {
            print("  Enter Actual Return Date (DD/MM/YYYY) (or '0' to cancel): ");
            getline(cin, returnDate);
            if (returnDate == "0") return;
            if (tool::date::isValidFormat(returnDate)) {
                break;
            }
            println("  [Error] Invalid date format. Please use DD/MM/YYYY.");
        }

        // Prompt for condition (Excellent/Good/Fair/Poor/Damaged)
        string condition;
        while (true) {
            print("  Enter Return Condition (Excellent/Good/Fair/Poor/Damaged) (or '0' to cancel): ");
            getline(cin, condition);
            if (condition == "0") return;
            
            if (condition == "Excellent" || condition == "Good" || condition == "Fair" || condition == "Poor" || condition == "Damaged") {
                break;
            }
            println("  [Error] Invalid condition. Must be Excellent, Good, Fair, Poor, or Damaged.");
        }

        println("\n  Processing return...");
        auto result = ::transaction::rental::processCostumeReturn(rentalId, returnDate, condition);
        if (result) {
            println("{}", result.value());
        } else {
            println("  [Error] Failed to process return: {}", result.error());
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
