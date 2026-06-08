#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/Apparel/Apparel.h"
#include "identity/Profile/Profile.h"
#include "inventory/InventoryUI.h"
#include "tool/DateHelper.h"
#include "transaction/Rental/Rental.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
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
        println("  [1] Manage Apparel Inventory");
        println("  [2] Manage Active Rentals & Returns");
        println("  [3] View Staff Profile");
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
                    manageApparelInventory(session);
                    break;
                case 2:
                    manageRentalsAndReturns(session);
                    break;
                case 3:
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

    static bool displayCatalogItemsHelper(const string& catalogUniqueId) {
        auto& db = database::DatabaseManager::getInstance();
        
        // Fetch the catalog name to make the header nice
        std::string nameQuery = std::format(
            "SELECT catalog_id, name, unique_id FROM apparel_catalog WHERE (unique_id = '{}' OR catalog_id = '{}') AND is_deleted = 0",
            catalogUniqueId, catalogUniqueId
        );
        auto nameRes = db.executeQuery(nameQuery);
        if (!nameRes) return false;
        
        sql::ResultSet* nrs = nameRes.value();
        if (!nrs->next()) {
            delete nrs;
            std::print("  [Error] Catalog ID '{}' not found.\n", catalogUniqueId);
            return false;
        }
        int catalogId = nrs->getInt("catalog_id");
        std::string catalogName = nrs->getString("name");
        std::string catalogUid = nrs->getString("unique_id");
        delete nrs;
        
        std::string query = std::format(
            "SELECT item_id, unique_id, size, status, condition_status FROM apparel_item "
            "WHERE catalog_id = {} AND is_deleted = 0",
            catalogId
        );
        auto result = db.executeQuery(query);
        if (!result) return false;
        
        sql::ResultSet* rs = result.value();
        std::print("\n  --- PHYSICAL ITEMS FOR CATALOG #{} ({}) ---\n", catalogUid, catalogName);
        vector<int> colWidths = {12, 10, 15, 15};
        tool::ui::printRow(colWidths, {"ITEM ID", "SIZE", "STATUS", "CONDITION"});
        tool::helper::drawLine(55, '-');
        
        bool found = false;
        while (rs->next()) {
            found = true;
            tool::ui::printRow(colWidths, {
                rs->getString("unique_id"),
                rs->getString("size"),
                rs->getString("status"),
                rs->getString("condition_status")
            });
        }
        delete rs;
        tool::helper::drawLine(55, '-');
        
        if (!found) {
            std::print("  No physical items registered for this catalog.\n");
        }
        return found;
    }

    static void updateConditionFlow() {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("UPDATE APPAREL CONDITION", 64);
        tool::helper::drawLine(64, '=');
        println("");

        int itemId = -1;
        string itemUniqueId = "";
        while (true) {
            print("  Enter Item ID to update (or 'S' to search by Catalog, '0' to cancel): ");
            string input;
            getline(cin, input);
            
            if (input == "0") return;
            if (input == "S" || input == "s") {
                print("  Enter Catalog ID to view physical items: ");
                string catalogInput;
                getline(cin, catalogInput);
                displayCatalogItemsHelper(catalogInput);
                println("");
                continue;
            }
            
            auto& db = database::DatabaseManager::getInstance();
            std::string q = std::format("SELECT item_id, unique_id FROM apparel_item WHERE (unique_id = '{}' OR item_id = '{}') AND is_deleted = 0", input, input);
            auto r = db.executeQuery(q);
            if (r) {
                sql::ResultSet* rs = r.value();
                if (rs->next()) {
                    itemId = rs->getInt("item_id");
                    itemUniqueId = rs->getString("unique_id");
                }
                delete rs;
            }
            
            if (itemId != -1) {
                break;
            } else {
                println("  [Error] Invalid Item ID or not found. Please try again.");
            }
        }

        println("  [1] Excellent");
        println("  [2] Good");
        println("  [3] Fair");
        println("  [4] Poor");
        println("  [5] Damaged");
        println("  [6] Laundry");
        print("  Select new condition / status option: ");

        int opt;
        if (!(cin >> opt)) {
            cin.clear();
            cin.ignore(1000, '\n');
            return;
        }
        cin.ignore(1000, '\n');

        if (opt == 6) {
            auto result = inventory::apparel::updateItemStatus(itemId, "Laundry");
            if (result) {
                std::print("\n  Status for Item #{} successfully updated to 'Laundry'.\n", itemUniqueId);
            } else {
                std::print("\n  Error: {}\n", result.error());
            }
        } else {
            string condition = "";
            switch(opt) {
                case 1: condition = "Excellent"; break;
                case 2: condition = "Good"; break;
                case 3: condition = "Fair"; break;
                case 4: condition = "Poor"; break;
                case 5: condition = "Damaged"; break;
                default: std::print("  Invalid option.\n"); return;
            }

            auto result = inventory::apparel::updateItemCondition(itemId, condition);
            if (result) {
                std::print("\n  Condition for Item #{} updated to '{}'.\n", itemUniqueId, condition);
            } else {
                std::print("\n  Error: {}\n", result.error());
            }
        }

        string waitInput;
        do {
            print("\nEnter '0' to return: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    static void retireApparelFlow() {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("RETIRE / REMOVE DAMAGED APPAREL", 64);
        tool::helper::drawLine(64, '=');
        println("");

        int itemId = -1;
        string itemUniqueId = "";
        while (true) {
            print("  Enter Item ID to retire (or 'S' to search by Catalog, '0' to cancel): ");
            string input;
            getline(cin, input);
            
            if (input == "0") return;
            if (input == "S" || input == "s") {
                print("  Enter Catalog ID to view physical items: ");
                string catalogInput;
                getline(cin, catalogInput);
                displayCatalogItemsHelper(catalogInput);
                println("");
                continue;
            }
            
            auto& db = database::DatabaseManager::getInstance();
            std::string q = std::format("SELECT item_id, unique_id FROM apparel_item WHERE (unique_id = '{}' OR item_id = '{}') AND is_deleted = 0", input, input);
            auto r = db.executeQuery(q);
            if (r) {
                sql::ResultSet* rs = r.value();
                if (rs->next()) {
                    itemId = rs->getInt("item_id");
                    itemUniqueId = rs->getString("unique_id");
                }
                delete rs;
            }
            
            if (itemId != -1) {
                break;
            } else {
                println("  [Error] Invalid Item ID or not found. Please try again.");
            }
        }

        // Fetch details of the item to confirm retirement
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "SELECT i.item_id, i.unique_id, c.name, i.size, i.status, i.condition_status "
            "FROM apparel_item i "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "WHERE i.item_id = {} AND i.is_deleted = 0 LIMIT 1",
            itemId
        );
        auto result = db.executeQuery(query);
        if (!result) {
            std::print("  [Error] Database lookup failed: {}\n", result.error());
            return;
        }

        sql::ResultSet* rs = result.value();
        if (!rs->next()) {
            delete rs;
            std::print("  [Error] Item ID #{} not found or already retired.\n", itemUniqueId);
            string waitInput;
            do {
                print("\nEnter '0' to return: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        std::string itemName = rs->getString("name");
        std::string itemSize = rs->getString("size");
        std::string itemStatus = rs->getString("status");
        std::string itemCondition = rs->getString("condition_status");
        delete rs;

        std::print("\n  --- ITEM TO RETIRE ---\n");
        std::print("  Item ID   : {}\n", itemUniqueId);
        std::print("  Name      : {}\n", itemName);
        std::print("  Size      : {}\n", itemSize);
        std::print("  Status    : {}\n", itemStatus);
        std::print("  Condition : {}\n", itemCondition);
        std::print("\n");

        print("  Are you sure you want to permanently retire this item? (Y/N): ");
        string confirm;
        getline(cin, confirm);

        if (confirm == "Y" || confirm == "y") {
            std::string retireQuery = std::format(
                "UPDATE apparel_item SET is_deleted = 1, status = 'Retired' WHERE item_id = {}",
                itemId
            );
            auto retireRes = db.executeUpdate(retireQuery);
            if (retireRes) {
                std::print("\n  Success: Item #{} has been successfully retired from circulation.\n", itemUniqueId);
            } else {
                std::print("\n  [Error] Failed to retire item: {}\n", retireRes.error());
            }
        } else {
            std::print("\n  Retirement cancelled.\n");
        }

        string waitInput;
        do {
            print("\nEnter '0' to return: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    static void processLaundryFlow() {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("PROCESS LAUNDRY", 64);
        tool::helper::drawLine(64, '=');
        println("");

        auto result = inventory::apparel::getItemsByStatus("Laundry");
        if (!result) {
            println("  Error fetching laundry items: {}", result.error());
            string waitInput;
            do {
                print("\nEnter '0' to return: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        auto items = result.value();
        if (items.empty()) {
            println("  No items currently in laundry.");
            string waitInput;
            do {
                print("\nEnter '0' to return: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        println("  ITEMS IN LAUNDRY:");
        for (const auto& item : items) {
            println("  Item ID: {} | Size: {} | Condition: {}", item.unique_id, item.size, item.condition_status);
        }
        tool::helper::drawLine(64, '-');

        print("  Enter Item ID to mark as washed (0 to cancel): ");
        string laundryInput;
        getline(cin, laundryInput);
        if (laundryInput.empty() || laundryInput == "0") {
            return;
        }

        // Resolve item unique ID
        int itemId = -1;
        string itemUniqueId = "";
        auto& db = database::DatabaseManager::getInstance();
        std::string q = std::format("SELECT item_id, unique_id FROM apparel_item WHERE (unique_id = '{}' OR item_id = '{}') AND is_deleted = 0", laundryInput, laundryInput);
        auto r = db.executeQuery(q);
        if (r) {
            sql::ResultSet* rs = r.value();
            if (rs->next()) {
                itemId = rs->getInt("item_id");
                itemUniqueId = rs->getString("unique_id");
            }
            delete rs;
        }

        if (itemId == -1) {
            println("  [Error] Item ID not found.");
            string waitInput;
            do {
                print("\nEnter '0' to return: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        auto updateRes = inventory::apparel::updateItemStatus(itemId, "Available");
        if (updateRes) {
            println("\n  Item #{} marked as Available.", itemUniqueId);
        } else {
            println("\n  Error: {}", updateRes.error());
        }

        string waitInput;
        do {
            print("\nEnter '0' to return: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void manageApparelInventory(const ::identity::auth::UserSession& session) {
        bool inInventoryMenu = true;
        while (inInventoryMenu) {
            tool::helper::clearScreen();
            tool::helper::drawLine(64, '=');
            tool::ui::displayTitle("INVENTORY MANAGEMENT", 64);
            tool::helper::drawLine(64, '=');
            println("");
            
            println("  [1] Register/Add New Apparel");
            println("  [2] View All Apparel (Full Catalog)");
            println("  [3] Update Apparel Condition / Status");
            println("  [4] Process Laundry & Maintenance");
            println("  [5] Retire/Remove Damaged Apparel");
            println("  [0] Back to Main Dashboard");
            
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");
            
            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');
            
            switch(choice) {
                case 1:
                    inventory::ui::registerNewApparel(session);
                    break;
                case 2:
                    inventory::ui::showCatalog(session);
                    break;
                case 3:
                    updateConditionFlow();
                    break;
                case 4:
                    processLaundryFlow();
                    break;
                case 5:
                    retireApparelFlow();
                    break;
                case 0:
                    inInventoryMenu = false;
                    break;
                default:
                    println("Invalid option.");
                    this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
    }

    void manageRentalsAndReturns(const ::identity::auth::UserSession& session) {
        bool inRentalsMenu = true;
        string searchTerm = "";
        int currentPage = 1;
        const int itemsPerPage = 25;

        while (inRentalsMenu) {
            tool::helper::clearScreen();
            tool::helper::drawLine(85, '=');
            tool::ui::displayTitle("ACTIVE & OVERDUE RENTALS MANAGEMENT", 85);
            tool::helper::drawLine(85, '=');
            if (!searchTerm.empty()) {
                println("  [Search Filter: '{}']", searchTerm);
            }
            println("");

            // Header for active rentals table
            vector<int> colWidths = {12, 18, 18, 5, 12, 10, 10};
            tool::ui::printRow(colWidths, {"ID", "CUSTOMER", "ITEM RENTED", "SIZE", "RETURN DATE", "RATE/DAY", "STATUS"});
            tool::helper::drawLine(85, '-');

            auto result = ::transaction::rental::getActiveRentals(searchTerm);
            if (result) {
                const auto& allItems = result.value();
                int totalItems = static_cast<int>(allItems.size());
                int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;

                if (currentPage > totalPages) currentPage = totalPages;
                if (currentPage < 1) currentPage = 1;

                int startIndex = (currentPage - 1) * itemsPerPage;
                int endIndex = min(startIndex + itemsPerPage, totalItems);

                if (totalItems == 0) {
                    println("  No active rentals found.");
                } else {
                    for (int i = startIndex; i < endIndex; ++i) {
                        const auto& item = allItems[i];
                        string statusStr = item.is_overdue ? "OVERDUE" : "Active";
                        tool::ui::printRow(colWidths, {
                            item.unique_id,
                            item.customer_name,
                            item.item_name,
                            item.size,
                            item.expected_return_date,
                            format("RM {:.2f}", item.daily_rate),
                            statusStr
                        });
                    }
                }

                tool::helper::drawLine(85, '=');
                
                // Pagination and Options Info
                if (totalPages > 1) {
                    string pageInfo = format("Page {} of {} | Total Outstanding: {}", currentPage, totalPages, totalItems);
                    int padding = (85 - static_cast<int>(pageInfo.length())) / 2;
                    string spaces(padding > 0 ? padding : 0, ' ');
                    println("{}{}", spaces, pageInfo);
                    tool::helper::drawLine(85, '-');
                    println("  [N] Next Page      [P] Previous Page      [S] Search Rentals");
                } else {
                    println("  [S] Search Rentals");
                }
                println("  [R] Process Return/Check-in Costume");
                println("  [0] Back to Main Dashboard");
                tool::helper::drawLine(85, '-');
                print("  Enter choice: ");

                string input;
                getline(cin, input);

                if (input == "0") {
                    inRentalsMenu = false;
                } else if (input == "N" || input == "n") {
                    if (currentPage < totalPages) currentPage++;
                } else if (input == "P" || input == "p") {
                    if (currentPage > 1) currentPage--;
                } else if (input == "S" || input == "s") {
                    print("  Enter search query (Customer or Apparel name): ");
                    getline(cin, searchTerm);
                    currentPage = 1;
                } else if (input == "R" || input == "r") {
                    // Inline Check-in costume return flow!
                    println("\n  --- PROCESS COSTUME RETURN ---");
                    string rentalIdInput;
                    print("  Enter Rental ID to process (or 0 to cancel): ");
                    getline(cin, rentalIdInput);
                    if (rentalIdInput.empty() || rentalIdInput == "0") {
                        continue;
                    }

                    // Prompt for actual return date with format validation
                    string returnDate;
                    while (true) {
                        print("  Enter Actual Return Date (DD/MM/YYYY) (or '0' to cancel): ");
                        getline(cin, returnDate);
                        if (returnDate == "0") break;
                        if (tool::date::isValidFormat(returnDate)) {
                            break;
                        }
                        println("  [Error] Invalid date format. Please use DD/MM/YYYY.");
                    }
                    if (returnDate == "0") continue;

                    // Prompt for condition (Excellent/Good/Fair/Poor/Damaged)
                    string condition;
                    while (true) {
                        print("  Enter Return Condition (Excellent/Good/Fair/Poor/Damaged) (or '0' to cancel): ");
                        getline(cin, condition);
                        if (condition == "0") break;
                        
                        if (condition == "Excellent" || condition == "Good" || condition == "Fair" || condition == "Poor" || condition == "Damaged") {
                            break;
                        }
                        println("  [Error] Invalid condition. Must be Excellent, Good, Fair, Poor, or Damaged.");
                    }
                    if (condition == "0") continue;

                    println("\n  Processing return...");
                    auto returnRes = ::transaction::rental::processCostumeReturn(rentalIdInput, returnDate, condition);
                    if (returnRes) {
                        println("{}", returnRes.value());
                    } else {
                        println("  [Error] Failed to process return: {}", returnRes.error());
                    }
                    string waitInput;
                    do {
                        print("\nEnter '0' to return: ");
                        getline(cin, waitInput);
                    } while (waitInput != "0");
                } else {
                    println("  Invalid choice.");
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            } else {
                println("  Error retrieving active rentals: {}", result.error());
                string waitInput;
                do {
                    print("\nEnter '0' to return to dashboard: ");
                    getline(cin, waitInput);
                } while (waitInput != "0");
                inRentalsMenu = false;
            }
        }
    }

    void viewStaffProfile(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("STAFF PROFILE", 64);
        tool::helper::drawLine(64, '=');
        println("");

        auto profileRes = ::identity::profile::Profile::getStaffProfile(session.userid);
        if (profileRes) {
            auto staff = profileRes.value();
            tool::ui::printField("Staff Name", staff.staff_name);
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Role", session.roles.front());
            tool::ui::printField("Position", staff.position);
            tool::ui::printField("Phone Number", staff.phone_no);
            
            // Query shop name dynamically using shop_id
            std::string shopName = "Not Assigned";
            if (staff.shop_id > 0) {
                auto& db = database::DatabaseManager::getInstance();
                std::string shopQuery = "SELECT shop_name FROM shops WHERE shop_id = " + to_string(staff.shop_id) + ";";
                auto shopRes = db.executeQuery(shopQuery);
                if (shopRes) {
                    sql::ResultSet* srs = shopRes.value();
                    if (srs->next()) {
                        shopName = srs->getString("shop_name");
                    }
                    delete srs;
                }
            }
            tool::ui::printField("Assigned Shop", shopName);
        } else {
            // Graceful fallback to formatted system values if profile is absent
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Role", session.roles.front());
            tool::ui::printField("Staff Email", session.username + "@utem.edu.my");
            tool::ui::printField("Department", "Rental Management");
            tool::ui::printField("Assigned Shop", "Not Assigned");
        }
        
        println("");
        println("  (Note: Contact admin to modify profile information)");
        tool::helper::drawLine(64, '-');
        
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }
}
