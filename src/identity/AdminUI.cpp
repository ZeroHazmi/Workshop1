#include "identity/AdminUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "transaction/Rental/AdminStats.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <print>
#include <string>
#include <iostream>
#include <thread>
#include <format>
#include <sstream>

using namespace std;

namespace identity::adminui {
    
    void showAdminDashboard(const ::identity::auth::UserSession& session) {
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
                    showStaffManagementSubmenu(session);
                    break;
                case 2:
                    showShopManagementSubmenu(session);
                    break;
                case 3:
                    showBusinessStatsSubmenu(session);
                    break;
                case 4:
                    viewAdminProfile(session);
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
                    this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
    }

    void registerStaffAccount() {
        tool::helper::clearScreen();

        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("REGISTER STAFF ACCOUNT", 64);
        tool::helper::drawLine(64, '=');

        println("");

        // Use the new registration flow with staff role
        ::identity::auth::Auth::handleRegisterFlow("staff");
        
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void manageStaffAccount() {
        tool::helper::clearScreen();

        tool::helper::drawLine(78, '=');
        tool::ui::displayTitle("MANAGE STAFF ACCOUNT", 78);
        tool::helper::drawLine(78, '=');

        println("");

        // 1. Fetch and display all registered staff in a formatted table
        auto& db = database::DatabaseManager::getInstance();
        std::string primaryQuery = 
            "SELECT s.staff_id, s.user_id, s.staff_name, u.username, s.position, s.phone_no, s.shop_id, "
            "       COALESCE(sh.shop_name, CONCAT('Shop #', s.shop_id)) AS shop_name "
            "FROM STAFF s "
            "JOIN USERS u ON s.user_id = u.user_id "
            "LEFT JOIN shops sh ON s.shop_id = sh.shop_id "
            "WHERE s.is_deleted = 0 "
            "ORDER BY s.staff_id ASC;";

        auto result = db.executeQuery(primaryQuery);
        if (!result) {
            // Graceful fallback query if shops left join fails
            std::string fallbackQuery = 
                "SELECT s.staff_id, s.user_id, s.staff_name, u.username, s.position, s.phone_no, s.shop_id, "
                "       CONCAT('Shop #', s.shop_id) AS shop_name "
                "FROM STAFF s "
                "JOIN USERS u ON s.user_id = u.user_id "
                "WHERE s.is_deleted = 0 "
                "ORDER BY s.staff_id ASC;";
            result = db.executeQuery(fallbackQuery);
        }

        if (!result) {
            cout << "  [Error] Failed to fetch staff list: " << result.error() << "\n";
        } else {
            sql::ResultSet* rs = result.value();
            vector<int> colWidths = {6, 18, 14, 14, 12, 14};
            
            tool::ui::printRow(colWidths, {"ID", "STAFF NAME", "USERNAME", "POSITION", "PHONE NO", "SHOP ASSIGNED"});
            tool::helper::drawLine(78, '-');

            bool hasStaff = false;
            while (rs->next()) {
                hasStaff = true;
                string idStr = to_string(rs->getInt("staff_id"));
                string nameStr = rs->getString("staff_name");
                string userStr = rs->getString("username");
                string posStr = rs->getString("position");
                string phoneStr = rs->getString("phone_no");
                string shopStr = rs->getString("shop_name");

                tool::ui::printRow(colWidths, {idStr, nameStr, userStr, posStr, phoneStr, shopStr});
            }
            delete rs;

            if (!hasStaff) {
                println("  No staff accounts registered in the database.");
            }
            tool::helper::drawLine(78, '=');
            println("");
        }

        int staffId;
        print("  Enter Staff ID to manage (or 0 to cancel): ");
        if (!(cin >> staffId) || staffId == 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            return;
        }
        cin.ignore(1000, '\n');

        // 2. Fetch specific staff info to verify existence and print details
        std::string checkQuery = "SELECT s.staff_id, s.user_id, s.staff_name, s.position, s.phone_no, s.shop_id, "
                                 "       COALESCE(sh.shop_name, CONCAT('Shop #', s.shop_id)) AS shop_name "
                                 "FROM STAFF s "
                                 "LEFT JOIN shops sh ON s.shop_id = sh.shop_id "
                                 "WHERE s.staff_id = " + to_string(staffId) + " AND s.is_deleted = 0;";

        auto checkRes = db.executeQuery(checkQuery);
        if (!checkRes) {
            checkQuery = "SELECT staff_id, user_id, staff_name, position, phone_no, shop_id, "
                         "       CONCAT('Shop #', shop_id) AS shop_name "
                         "FROM STAFF "
                         "WHERE staff_id = " + to_string(staffId) + " AND is_deleted = 0;";
            checkRes = db.executeQuery(checkQuery);
        }

        if (!checkRes) {
            cout << "  [Error] Database error: " << checkRes.error() << "\n";
            return;
        }

        sql::ResultSet* crs = checkRes.value();
        if (!crs->next()) {
            delete crs;
            println("  [Error] Staff ID not found.");
            this_thread::sleep_for(chrono::milliseconds(1500));
            return;
        }

        string staffName = crs->getString("staff_name");
        string position = crs->getString("position");
        string phone = crs->getString("phone_no");
        string shopName = crs->getString("shop_name");
        int shopId = crs->getInt("shop_id");
        int userId = crs->getInt("user_id");
        delete crs;

        println("\n  Managing Staff: {} (ID: {})", staffName, staffId);
        println("  Current Position: {}", position);
        println("  Assigned Branch : {}", shopName);
        tool::helper::drawLine(74, '-');

        // Display staff management options
        println("\n  [1] Update Staff Information");
        println("  [2] Change Shop Assignment");
        println("  [3] Remove / Terminate Staff Account (Soft Delete)");
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
                println("\n  --- UPDATE STAFF INFORMATION ---");
                string newName, newPosition, newPhone;
                print("  Enter New Name (leave blank to keep current): ");
                getline(cin, newName);
                print("  Enter New Position (leave blank to keep current): ");
                getline(cin, newPosition);
                print("  Enter New Phone (leave blank to keep current): ");
                getline(cin, newPhone);

                if (newName.empty()) newName = staffName;
                if (newPosition.empty()) newPosition = position;
                if (newPhone.empty()) newPhone = phone;

                std::string updateQuery = "UPDATE STAFF SET staff_name = '" + newName + 
                                          "', position = '" + newPosition + 
                                          "', phone_no = '" + newPhone + 
                                          "' WHERE staff_id = " + to_string(staffId) + ";";

                auto updateRes = db.executeUpdate(updateQuery);
                if (updateRes) {
                    println("\n  Success: Staff information updated successfully.");
                } else {
                    cout << "  [Error] Failed to update staff: " << updateRes.error() << "\n";
                }
                break;
            }
            case 2: {
                println("\n  --- CHANGE SHOP ASSIGNMENT ---");
                int newShopId;
                print("  Enter New Shop ID to assign: ");
                if (!(cin >> newShopId)) {
                    cin.clear();
                    cin.ignore(1000, '\n');
                    println("  Invalid input. Operation cancelled.");
                    break;
                }
                cin.ignore(1000, '\n');

                std::string updateQuery = "UPDATE STAFF SET shop_id = " + to_string(newShopId) + 
                                          " WHERE staff_id = " + to_string(staffId) + ";";

                auto updateRes = db.executeUpdate(updateQuery);
                if (updateRes) {
                    println("\n  Success: Shop assignment updated successfully.");
                } else {
                    cout << "  [Error] Failed to update assignment: " << updateRes.error() << "\n";
                }
                break;
            }
            case 3: {
                println("\n  --- TERMINATE STAFF ACCOUNT ---");
                print("  Are you sure you want to soft delete this staff account? (Y/N): ");
                string confirm;
                getline(cin, confirm);
                if (confirm != "Y" && confirm != "y") {
                    println("  Termination cancelled.");
                    break;
                }

                // 1. Soft delete in STAFF
                std::string deleteStaffQuery = "UPDATE STAFF SET is_deleted = 1 WHERE staff_id = " + to_string(staffId) + ";";
                auto resStaff = db.executeUpdate(deleteStaffQuery);

                // 2. Soft delete in USERS
                std::string deleteUserQuery = "UPDATE USERS SET is_deleted = 1 WHERE user_id = " + to_string(userId) + ";";
                auto resUser = db.executeUpdate(deleteUserQuery);

                if (resStaff && resUser) {
                    println("\n  Success: Staff account and linked user login soft deleted successfully.");
                } else {
                    cout << "  [Warning] Partial failure during deletion.\n";
                }
                break;
            }
            case 0:
                return;
            default:
                println("  Invalid option.");
        }

        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void viewAdminProfile(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();

        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("ADMIN PROFILE", 64);
        tool::helper::drawLine(64, '=');
        println("");

        tool::ui::printField("Username", session.username);
        tool::ui::printField("Role", session.roles.front());
        tool::ui::printField("Admin Email", session.username + "@utem.edu.my");
        tool::ui::printField("Department", "FWCRS System Administration");
        
        println("");
        tool::helper::drawLine(64, '-');
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void registerNewShop() {
        tool::helper::clearScreen();

        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("REGISTER NEW SHOP", 64);
        tool::helper::drawLine(64, '=');

        println("");

        string shopName, location, phone;
        print("  Shop Name: ");
        getline(cin, shopName);
        
        print("  Location: ");
        getline(cin, location);
        
        print("  Contact Phone: ");
        getline(cin, phone);

        if (shopName.empty() || location.empty()) {
            println("\n  [Error] Shop name and location are required.");
            this_thread::sleep_for(chrono::milliseconds(1500));
            return;
        }

        auto& db = database::DatabaseManager::getInstance();
        std::string query = "INSERT INTO shops (shop_name, location, shop_phone) VALUES ('" + 
                            shopName + "', '" + location + "', '" + phone + "');";
        
        auto res = db.executeUpdate(query);
        if (res) {
            println("\n  Shop registered successfully!");
        } else {
            cout << "  [Error] Failed to register shop: " << res.error() << "\n";
        }
        
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void displayShopList() {
        tool::helper::clearScreen();

        tool::helper::drawLine(68, '=');
        tool::ui::displayTitle("SHOP LIST", 68);
        tool::helper::drawLine(68, '=');
        println("");

        vector<int> colWidths = {6, 25, 20, 15};
        tool::ui::printRow(colWidths, {"ID", "SHOP NAME", "LOCATION", "CONTACT PHONE"});
        tool::helper::drawLine(68, '-');

        auto& db = database::DatabaseManager::getInstance();
        std::string query = "SELECT shop_id, shop_name, location, shop_phone FROM shops ORDER BY shop_id ASC;";
        auto result = db.executeQuery(query);

        if (!result) {
            cout << "  [Error] Failed to fetch shops: " << result.error() << "\n";
        } else {
            sql::ResultSet* rs = result.value();
            bool hasShops = false;
            while (rs->next()) {
                hasShops = true;
                string idStr = to_string(rs->getInt("shop_id"));
                string nameStr = rs->getString("shop_name");
                string locStr = rs->getString("location");
                string phoneStr = rs->getString("shop_phone");
                tool::ui::printRow(colWidths, {idStr, nameStr, locStr, phoneStr});
            }
            delete rs;

            if (!hasShops) {
                println("  No shops registered in the database.");
            }
        }
        tool::helper::drawLine(68, '=');
    }

    void manageShopInformation() {
        bool managing = true;
        while (managing) {
            displayShopList();
            println("");

            int shopId;
            print("  Enter Shop ID to manage (or 0 to cancel/back): ");
            if (!(cin >> shopId) || shopId == 0) {
                cin.clear();
                cin.ignore(1000, '\n');
                return;
            }
            cin.ignore(1000, '\n');

            auto& db = database::DatabaseManager::getInstance();
            std::string query = "SELECT shop_id, shop_name, location, shop_phone FROM shops WHERE shop_id = " + to_string(shopId) + ";";
            auto checkRes = db.executeQuery(query);
            if (!checkRes) {
                cout << "  [Error] Database error: " << checkRes.error() << "\n";
                this_thread::sleep_for(chrono::milliseconds(1500));
                continue;
            }

            sql::ResultSet* crs = checkRes.value();
            if (!crs->next()) {
                delete crs;
                println("  [Error] Shop ID not found.");
                this_thread::sleep_for(chrono::milliseconds(1500));
                continue;
            }

            string shopName = crs->getString("shop_name");
            string location = crs->getString("location");
            string phone = crs->getString("shop_phone");
            delete crs;

            println("\n  Managing Shop: {} (ID: {})", shopName, shopId);
            println("  Location: {}", location);
            println("  Contact : {}", phone);
            tool::helper::drawLine(68, '-');

            println("\n  [1] Update Shop Name");
            println("  [2] Update Location");
            println("  [3] Update Contact Phone");
            println("  [0] Back to Menu");
            print("  Select option: ");

            int option;
            if (!(cin >> option)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');

            switch (option) {
                case 1: {
                    print("\n  Enter New Shop Name: ");
                    string newName;
                    getline(cin, newName);
                    if (newName.empty()) break;

                    std::string updateQuery = "UPDATE shops SET shop_name = '" + newName + "' WHERE shop_id = " + to_string(shopId) + ";";
                    auto res = db.executeUpdate(updateQuery);
                    if (res) {
                        println("  Success: Shop name updated.");
                    } else {
                        cout << "  [Error] Failed to update: " << res.error() << "\n";
                    }
                    this_thread::sleep_for(chrono::milliseconds(1200));
                    break;
                }
                case 2: {
                    print("\n  Enter New Location: ");
                    string newLoc;
                    getline(cin, newLoc);
                    if (newLoc.empty()) break;

                    std::string updateQuery = "UPDATE shops SET location = '" + newLoc + "' WHERE shop_id = " + to_string(shopId) + ";";
                    auto res = db.executeUpdate(updateQuery);
                    if (res) {
                        println("  Success: Location updated.");
                    } else {
                        cout << "  [Error] Failed to update: " << res.error() << "\n";
                    }
                    this_thread::sleep_for(chrono::milliseconds(1200));
                    break;
                }
                case 3: {
                    print("\n  Enter New Contact Phone: ");
                    string newPhone;
                    getline(cin, newPhone);
                    if (newPhone.empty()) break;

                    std::string updateQuery = "UPDATE shops SET shop_phone = '" + newPhone + "' WHERE shop_id = " + to_string(shopId) + ";";
                    auto res = db.executeUpdate(updateQuery);
                    if (res) {
                        println("  Success: Contact phone updated.");
                    } else {
                        cout << "  [Error] Failed to update: " << res.error() << "\n";
                    }
                    this_thread::sleep_for(chrono::milliseconds(1200));
                    break;
                }
                case 0:
                    break;
                default:
                    println("  Invalid option.");
                    this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
    }

    void viewShopInventory() {
        displayShopList();
        println("");

        int shopId;
        print("  Enter Shop ID to view inventory (or 0 to cancel): ");
        if (!(cin >> shopId) || shopId == 0) {
            cin.clear();
            cin.ignore(1000, '\n');
            return;
        }
        cin.ignore(1000, '\n');

        // Verify shop existence first
        auto& db = database::DatabaseManager::getInstance();
        std::string checkQuery = "SELECT shop_name FROM shops WHERE shop_id = " + to_string(shopId) + ";";
        auto checkRes = db.executeQuery(checkQuery);
        if (!checkRes) {
            cout << "  [Error] Database error: " << checkRes.error() << "\n";
            return;
        }
        sql::ResultSet* crs = checkRes.value();
        if (!crs->next()) {
            delete crs;
            println("  [Error] Shop ID not found.");
            this_thread::sleep_for(chrono::milliseconds(1500));
            return;
        }
        string shopName = crs->getString("shop_name");
        delete crs;

        println("\n  Inventory for: {} (ID: {})\n", shopName, shopId);

        vector<int> colWidths = {6, 25, 12, 12};
        tool::ui::printRow(colWidths, {"CAT ID", "ITEM NAME", "QUANTITY", "CONDITION"});
        tool::helper::drawLine(65, '-');

        std::string query = 
            "SELECT c.catalog_id, c.name, COUNT(i.item_id) AS quantity, i.condition_status "
            "FROM apparel_catalog c "
            "JOIN apparel_item i ON c.catalog_id = i.catalog_id "
            "WHERE c.shop_id = " + to_string(shopId) + " AND i.is_deleted = 0 AND c.is_deleted = 0 "
            "GROUP BY c.catalog_id, c.name, i.condition_status "
            "ORDER BY c.catalog_id ASC;";

        auto result = db.executeQuery(query);
        if (!result) {
            cout << "  [Error] Failed to fetch inventory: " << result.error() << "\n";
        } else {
            sql::ResultSet* rs = result.value();
            bool hasItems = false;
            while (rs->next()) {
                hasItems = true;
                string catIdStr = to_string(rs->getInt("catalog_id"));
                string nameStr = rs->getString("name");
                string qtyStr = to_string(rs->getInt("quantity"));
                string condStr = rs->getString("condition_status");
                tool::ui::printRow(colWidths, {catIdStr, nameStr, qtyStr, condStr});
            }
            delete rs;

            if (!hasItems) {
                println("  No items currently in inventory for this shop branch.");
            }
        }

        tool::helper::drawLine(65, '=');
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

    void showStaffManagementSubmenu(const ::identity::auth::UserSession& session) {
        bool inSubmenu = true;
        while (inSubmenu) {
            tool::helper::clearScreen();
            tool::helper::drawLine(64, '=');
            tool::ui::displayTitle("STAFF MANAGEMENT", 64);
            tool::helper::drawLine(64, '=');
            println("");
            println("  [1] Register Staff Account");
            println("  [2] Manage Staff Account");
            println("  [0] Return to Dashboard");
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");

            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');

            switch (choice) {
                case 1:
                    registerStaffAccount();
                    break;
                case 2:
                    manageStaffAccount();
                    break;
                case 0:
                    inSubmenu = false;
                    break;
                default:
                    println("Invalid option. Press Enter to continue...");
                    cin.get();
            }
        }
    }

    void showShopManagementSubmenu(const ::identity::auth::UserSession& session) {
        bool inSubmenu = true;
        while (inSubmenu) {
            tool::helper::clearScreen();
            tool::helper::drawLine(64, '=');
            tool::ui::displayTitle("SHOP & INVENTORY MANAGEMENT", 64);
            tool::helper::drawLine(64, '=');
            println("");
            println("  [1] Register New Shop");
            println("  [2] Display & Manage Shop Information");
            println("  [3] View Shop/Branch Inventory");
            println("  [0] Return to Dashboard");
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");

            int choice;
            if (!(cin >> choice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');

            switch (choice) {
                case 1:
                    registerNewShop();
                    break;
                case 2:
                    manageShopInformation();
                    break;
                case 3:
                    viewShopInventory();
                    break;
                case 0:
                    inSubmenu = false;
                    break;
                default:
                    println("Invalid option. Press Enter to continue...");
                    cin.get();
            }
        }
    }

    bool isValidDate(const std::string& dateStr) {
        if (dateStr.length() != 10) return false;
        if (dateStr[4] != '-' || dateStr[7] != '-') return false;
        for (int i = 0; i < 10; ++i) {
            if (i == 4 || i == 7) continue;
            if (!isdigit(dateStr[i])) return false;
        }
        return true;
    }

    void updateActiveDateRange(transaction::rental::stats::DateRange& activeDateRange, std::string& activeDateRangeLabel) {
        println("");
        println("  Select Time Range Option:");
        println("  [1] Last 3 Months");
        println("  [2] Last 6 Months (Default)");
        println("  [3] Last 12 Months");
        println("  [4] Custom Date Range");
        println("  [0] Clear Date Filter (All Time)");
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
        
        string choice;
        getline(cin, choice);
        
        auto& db = database::DatabaseManager::getInstance();
        
        if (choice == "1") {
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 3 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                    activeDateRangeLabel = "Last 3 Months";
                    print("\n  Filter updated to: Last 3 Months ({} to {})\n", activeDateRange.startDate, activeDateRange.endDate);
                }
                delete rs;
            } else {
                print("\n  [Error] Failed to calculate dates in database: {}\n", result.error());
            }
        } else if (choice == "2") {
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 6 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                    activeDateRangeLabel = "Last 6 Months";
                    print("\n  Filter updated to: Last 6 Months ({} to {})\n", activeDateRange.startDate, activeDateRange.endDate);
                }
                delete rs;
            } else {
                print("\n  [Error] Failed to calculate dates in database: {}\n", result.error());
            }
        } else if (choice == "3") {
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 12 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                    activeDateRangeLabel = "Last 12 Months";
                    print("\n  Filter updated to: Last 12 Months ({} to {})\n", activeDateRange.startDate, activeDateRange.endDate);
                }
                delete rs;
            } else {
                print("\n  [Error] Failed to calculate dates in database: {}\n", result.error());
            }
        } else if (choice == "4") {
            print("  Enter Start Date (YYYY-MM-DD): ");
            string startInput;
            getline(cin, startInput);
            
            print("  Enter End Date (YYYY-MM-DD): ");
            string endInput;
            getline(cin, endInput);
            
            // Trim inputs
            startInput.erase(0, startInput.find_first_not_of(" \t\r\n"));
            startInput.erase(startInput.find_last_not_of(" \t\r\n") + 1);
            endInput.erase(0, endInput.find_first_not_of(" \t\r\n"));
            endInput.erase(endInput.find_last_not_of(" \t\r\n") + 1);
            
            if (!isValidDate(startInput) || !isValidDate(endInput)) {
                println("\n  [Error] Invalid date format. Must be strictly YYYY-MM-DD.");
            } else if (startInput > endInput) {
                println("\n  [Error] Start date cannot be after End date.");
            } else {
                activeDateRange.startDate = startInput;
                activeDateRange.endDate = endInput;
                activeDateRangeLabel = "Custom (" + startInput + " to " + endInput + ")";
                print("\n  Filter updated to: Custom range ({} to {})\n", startInput, endInput);
            }
        } else if (choice == "0") {
            activeDateRange.startDate = "";
            activeDateRange.endDate = "";
            activeDateRangeLabel = "All Time";
            println("\n  Date range filter cleared (All Time).");
        } else {
            println("\n  Invalid option. Preserving previous timeframe.");
        }
        
        this_thread::sleep_for(chrono::milliseconds(1500));
    }

    void updateActiveFilters(std::vector<int>& activeShopIds, std::vector<string>& activeShopNames) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = "SELECT shop_id, shop_name FROM shops ORDER BY shop_id ASC;";
        auto result = db.executeQuery(query);
        println("");
        if (result) {
            sql::ResultSet* rs = result.value();
            print("  Available Shops: ");
            bool first = true;
            while (rs->next()) {
                if (!first) print(" | ");
                print("{}: {}", rs->getInt("shop_id"), string(rs->getString("shop_name")));
                first = false;
            }
            println("");
            delete rs;
        }
        
        print("  Enter Shop ID(s) to filter by, separated by commas (e.g. 1, 3 or 0 for All): ");
        string input;
        getline(cin, input);
        
        // Parse input
        std::vector<int> newIds;
        std::stringstream ss(input);
        std::string token;
        bool hasZero = false;
        while (std::getline(ss, token, ',')) {
            // Trim token
            token.erase(0, token.find_first_not_of(" \t\r\n"));
            token.erase(token.find_last_not_of(" \t\r\n") + 1);
            if (token.empty()) continue;
            try {
                int id = std::stoi(token);
                if (id == 0) {
                    hasZero = true;
                } else {
                    newIds.push_back(id);
                }
            } catch (...) {
                // Ignore invalid input tokens
            }
        }
        
        if (hasZero || newIds.empty()) {
            activeShopIds.clear();
            activeShopNames.clear();
            println("\n  Filters cleared. Showing all shops.");
        } else {
            // Validate and fetch shop names
            std::vector<int> validatedIds;
            std::vector<string> validatedNames;
            for (int id : newIds) {
                std::string checkQuery = "SELECT shop_name FROM shops WHERE shop_id = " + to_string(id) + ";";
                auto checkRes = db.executeQuery(checkQuery);
                if (checkRes) {
                    sql::ResultSet* crs = checkRes.value();
                    if (crs->next()) {
                        validatedIds.push_back(id);
                        validatedNames.push_back(crs->getString("shop_name"));
                    }
                    delete crs;
                }
            }
            activeShopIds = validatedIds;
            activeShopNames = validatedNames;
            if (activeShopIds.empty()) {
                println("\n  No valid Shop IDs entered. Filters cleared.");
            } else {
                println("");
                print("  Active Filter set to: ");
                for (size_t i = 0; i < activeShopNames.size(); ++i) {
                    print("{}", activeShopNames[i]);
                    if (i + 1 < activeShopNames.size()) print(", ");
                }
                println("");
            }
        }
        this_thread::sleep_for(chrono::milliseconds(1500));
    }

    void showBusinessStatsSubmenu(const ::identity::auth::UserSession& session) {
        std::vector<int> activeShopIds;
        std::vector<string> activeShopNames;
        
        transaction::rental::stats::DateRange activeDateRange;
        std::string activeDateRangeLabel = "Last 6 Months";
        
        // Initialize default to Last 6 Months dynamically on first load
        {
            auto& db = database::DatabaseManager::getInstance();
            std::string q = "SELECT DATE_FORMAT(DATE_SUB(NOW(), INTERVAL 6 MONTH), '%Y-%m-%d') AS start, DATE_FORMAT(NOW(), '%Y-%m-%d') AS end;";
            auto result = db.executeQuery(q);
            if (result) {
                sql::ResultSet* rs = result.value();
                if (rs->next()) {
                    activeDateRange.startDate = rs->getString("start");
                    activeDateRange.endDate = rs->getString("end");
                }
                delete rs;
            }
        }
        
        bool inStats = true;
        while (inStats) {
            tool::helper::clearScreen();
            tool::helper::drawLine(64, '=');
            tool::ui::displayTitle("BUSINESS REPORTS & STATISTICS", 64);
            tool::helper::drawLine(64, '=');
            
            // Show active filters
            if (activeShopIds.empty()) {
                print("  Active Filter: All Shops");
            } else {
                print("  Active Filter: ");
                for (size_t i = 0; i < activeShopNames.size(); ++i) {
                    print("{}", activeShopNames[i]);
                    if (i + 1 < activeShopNames.size()) print(", ");
                }
            }
            print(" | Time Range: {}\n", activeDateRangeLabel);
            println("");
            
            println("  [1] Monthly Revenue Trends");
            println("  [2] Costume Popularity & Demand");
            println("  [3] Branch Performance & Revenue Share");
            println("  [4] Inventory Quality Audit");
            println("  [F] Toggle/Edit Branch Filters");
            println("  [T] Toggle/Edit Time Range");
            println("  [0] Return to Dashboard");
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");

            string choiceStr;
            getline(cin, choiceStr);
            // Trim choiceStr
            choiceStr.erase(0, choiceStr.find_first_not_of(" \t\r\n"));
            choiceStr.erase(choiceStr.find_last_not_of(" \t\r\n") + 1);

            if (choiceStr == "1") {
                bool inReport = true;
                while (inReport) {
                    tool::helper::clearScreen();
                    tool::helper::drawLine(64, '=');
                    tool::ui::displayTitle("MONTHLY REVENUE TRENDS", 64);
                    tool::helper::drawLine(64, '=');
                    
                    // Header Filter Status
                    if (activeShopIds.empty()) {
                        print("  Active Filter: All Shops");
                    } else {
                        print("  Active Filter: ");
                        for (size_t i = 0; i < activeShopNames.size(); ++i) {
                            print("{}", activeShopNames[i]);
                            if (i + 1 < activeShopNames.size()) print(", ");
                        }
                    }
                    print(" | Time Range: {}\n", activeDateRangeLabel);
                    println("");

                    auto trendsOpt = transaction::rental::stats::getRevenueTrends(activeShopIds, activeDateRange);
                    if (!trendsOpt) {
                        print("{}\n", format("  Error retrieving revenue trends: {}", trendsOpt.error()));
                    } else {
                        auto trends = trendsOpt.value();
                        if (trends.empty()) {
                            println("  No revenue data available for selected timeframe.");
                        } else {
                            // Find max revenue for scaling
                            double maxRev = 0.0;
                            for (const auto& pt : trends) {
                                if (pt.revenue > maxRev) maxRev = pt.revenue;
                            }

                            // Print vertical bar chart
                            int maxRows = 10;
                            for (int r = maxRows; r >= 1; --r) {
                                double threshold = maxRev * (r / (double)maxRows);
                                std::print("  {:9.2f} | ", threshold);
                                for (const auto& pt : trends) {
                                    if (pt.revenue >= threshold && pt.revenue > 0.0) {
                                        std::print("   ███   ");
                                    } else {
                                        std::print("         ");
                                    }
                                }
                                std::print("\n");
                            }

                            // Chart base line
                            std::print("            +");
                            for (size_t i = 0; i < trends.size(); ++i) {
                                std::print("---------");
                            }
                            std::print("\n             ");

                            // X-axis month labels formatted to exactly 6 characters (e.g., "May 26") and centered
                            for (const auto& pt : trends) {
                                std::string displayMonth = pt.month_name;
                                if (displayMonth.length() == 8 && displayMonth[3] == ' ') {
                                    displayMonth = displayMonth.substr(0, 3) + " " + displayMonth.substr(6, 2);
                                }
                                std::print("  {:^6} ", displayMonth);
                            }
                            print("\n\n");

                            // Tabular details
                            tool::helper::drawLine(64, '-');
                            print("{}\n", format("  {:<20} | {:<20}", "MONTH", "REVENUE (RM)"));
                            tool::helper::drawLine(64, '-');
                            for (const auto& pt : trends) {
                                print("{}\n", format("  {:<20} | RM {:<18.2f}", pt.month_name, pt.revenue));
                            }
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops, 'T' to change time range: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    } else if (waitInput == "T" || waitInput == "t") {
                        updateActiveDateRange(activeDateRange, activeDateRangeLabel);
                    }
                }
            } else if (choiceStr == "2") {
                bool inReport = true;
                while (inReport) {
                    tool::helper::clearScreen();
                    tool::helper::drawLine(64, '=');
                    tool::ui::displayTitle("COSTUME POPULARITY & DEMAND", 64);
                    tool::helper::drawLine(64, '=');
                    
                    // Header Filter Status
                    if (activeShopIds.empty()) {
                        print("  Active Filter: All Shops");
                    } else {
                        print("  Active Filter: ");
                        for (size_t i = 0; i < activeShopNames.size(); ++i) {
                            print("{}", activeShopNames[i]);
                            if (i + 1 < activeShopNames.size()) print(", ");
                        }
                    }
                    print(" | Time Range: {}\n", activeDateRangeLabel);
                    println("");

                    auto popOpt = transaction::rental::stats::getCostumePopularity(activeShopIds, activeDateRange);
                    if (!popOpt) {
                        print("{}\n", format("  Error retrieving costume popularity: {}", popOpt.error()));
                    } else {
                        auto points = popOpt.value();
                        if (points.empty()) {
                            println("  No rental data available for selected timeframe.");
                        } else {
                            int maxCount = 0;
                            for (const auto& pt : points) {
                                if (pt.rental_count > maxCount) maxCount = pt.rental_count;
                            }

                            println("  Top Popular Costumes by Rental Volume:");
                            println("");

                            int maxBarWidth = 25;
                            for (const auto& pt : points) {
                                int barWidth = maxCount > 0 ? (pt.rental_count * maxBarWidth / maxCount) : 0;
                                string unicodeBar = "";
                                for (int i = 0; i < barWidth; ++i) {
                                    unicodeBar += "█";
                                }
                                print("{}\n", format("  {:<25} | {:<25} ({} rentals)", pt.catalog_name, unicodeBar, pt.rental_count));
                            }
                            println("");
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops, 'T' to change time range: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    } else if (waitInput == "T" || waitInput == "t") {
                        updateActiveDateRange(activeDateRange, activeDateRangeLabel);
                    }
                }
            } else if (choiceStr == "3") {
                bool inReport = true;
                while (inReport) {
                    tool::helper::clearScreen();
                    tool::helper::drawLine(64, '=');
                    tool::ui::displayTitle("BRANCH PERFORMANCE & REVENUE SHARE", 64);
                    tool::helper::drawLine(64, '=');
                    
                    // Header Filter Status
                    if (activeShopIds.empty()) {
                        print("  Active Filter: All Shops");
                    } else {
                        print("  Active Filter: ");
                        for (size_t i = 0; i < activeShopNames.size(); ++i) {
                            print("{}", activeShopNames[i]);
                            if (i + 1 < activeShopNames.size()) print(", ");
                        }
                    }
                    print(" | Time Range: {}\n", activeDateRangeLabel);
                    println("");

                    auto branchOpt = transaction::rental::stats::getBranchPerformance(activeShopIds, activeDateRange);
                    if (!branchOpt) {
                        print("{}\n", format("  Error retrieving branch performance: {}", branchOpt.error()));
                    } else {
                        auto branches = branchOpt.value();
                        if (branches.empty()) {
                            println("  No branch revenue records found for selected timeframe.");
                        } else {
                            double totalRevenue = 0.0;
                            for (const auto& pt : branches) totalRevenue += pt.revenue;

                            print("{}\n\n", format("  Total Combined Revenue: RM {:.2f}", totalRevenue));
                            
                            if (totalRevenue > 0.0) {
                                for (const auto& pt : branches) {
                                    double pct = (pt.revenue / totalRevenue) * 100.0;
                                    int barChars = (int)(pct / 2.5); // 40 chars total
                                    string barStr = "";
                                    for (int i = 0; i < barChars; ++i) barStr += "█";
                                    string remStr = "";
                                    for (int i = 0; i < (40 - barChars); ++i) remStr += "░";
                                    
                                    print("{}\n", format("  {:<20} [ {}{} ] {:5.1f}% (RM {:.2f})", 
                                        pt.shop_name, barStr, remStr, pct, pt.revenue));
                                }
                            } else {
                                println("  No revenue generated yet.");
                            }
                            println("");
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops, 'T' to change time range: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    } else if (waitInput == "T" || waitInput == "t") {
                        updateActiveDateRange(activeDateRange, activeDateRangeLabel);
                    }
                }
            } else if (choiceStr == "4") {
                bool inReport = true;
                while (inReport) {
                    tool::helper::clearScreen();
                    tool::helper::drawLine(64, '=');
                    tool::ui::displayTitle("INVENTORY QUALITY AUDIT", 64);
                    tool::helper::drawLine(64, '=');
                    
                    // Header Filter Status (Time Range N/A since physical condition represents immediate current state)
                    if (activeShopIds.empty()) {
                        print("  Active Filter: All Shops");
                    } else {
                        print("  Active Filter: ");
                        for (size_t i = 0; i < activeShopNames.size(); ++i) {
                            print("{}", activeShopNames[i]);
                            if (i + 1 < activeShopNames.size()) print(", ");
                        }
                    }
                    print(" | Time Range: N/A (Current Physical State)\n");
                    println("");

                    auto auditOpt = transaction::rental::stats::getInventoryConditionAudit(activeShopIds);
                    if (!auditOpt) {
                        print("{}\n", format("  Error retrieving inventory audit: {}", auditOpt.error()));
                    } else {
                        auto conditions = auditOpt.value();
                        if (conditions.empty()) {
                            println("  No physical inventory items found in database.");
                        } else {
                            int totalItems = 0;
                            for (const auto& pt : conditions) totalItems += pt.count;

                            vector<int> colWidths = {25, 15, 15};
                            tool::helper::drawLine(64, '-');
                            tool::ui::printRow(colWidths, {"CONDITION STATUS", "ITEM COUNT", "PERCENTAGE"});
                            tool::helper::drawLine(64, '-');

                            for (const auto& pt : conditions) {
                                double pct = totalItems > 0 ? (pt.count / (double)totalItems) * 100.0 : 0.0;
                                string countStr = to_string(pt.count);
                                string pctStr = format("{:.1f}%", pct);
                                tool::ui::printRow(colWidths, {pt.condition, countStr, pctStr});
                            }
                            tool::helper::drawLine(64, '-');
                            string totalItemsStr = to_string(totalItems);
                            tool::ui::printRow(colWidths, {"TOTAL ITEMS", totalItemsStr, "100.0%"});
                            tool::helper::drawLine(64, '=');
                        }
                    }

                    print("\n  Enter '0' to return, 'F' to filter shops: ");
                    string waitInput;
                    getline(cin, waitInput);
                    if (waitInput == "0") {
                        inReport = false;
                    } else if (waitInput == "F" || waitInput == "f") {
                        updateActiveFilters(activeShopIds, activeShopNames);
                    }
                }
            } else if (choiceStr == "F" || choiceStr == "f") {
                updateActiveFilters(activeShopIds, activeShopNames);
            } else if (choiceStr == "T" || choiceStr == "t") {
                updateActiveDateRange(activeDateRange, activeDateRangeLabel);
            } else if (choiceStr == "0") {
                inStats = false;
            } else {
                println("Invalid option. Press Enter to continue...");
                cin.get();
            }
        }
    }
}
