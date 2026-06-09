#include "identity/AdminUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <print>
#include <string>
#include <iostream>
#include <thread>
#include <format>
#include <vector>

namespace db = ::database;
namespace auth = ::identity::auth;

using namespace std;

namespace identity::adminui {

    void registerNewShop() {
        tool::ui::showHeader("REGISTER NEW SHOP", 64);

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

        string uniqueId = db::DatabaseManager::generateUniqueId("SHP");
        auto& db = db::DatabaseManager::getInstance();
        string query = "INSERT INTO shops (shop_name, location, shop_phone, unique_id) VALUES ('" + 
                            shopName + "', '" + location + "', '" + phone + "', '" + uniqueId + "');";
        
        auto res = db.executeUpdate(query);
        if (res) {
            println("\n  Shop registered successfully!");
        } else {
            cout << "  [Error] Failed to register shop: " << res.error() << "\n";
        }
        
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

    void displayShopList() {
        tool::helper::clearScreen();

        tool::helper::drawLine(68, '=');
        tool::ui::displayTitle("SHOP LIST", 68);
        tool::helper::drawLine(68, '=');
        println("");

        vector<int> colWidths = {12, 22, 19, 15};
        tool::ui::printRow(colWidths, {"ID", "SHOP NAME", "LOCATION", "CONTACT PHONE"});
        tool::helper::drawLine(68, '-');

        auto& db = db::DatabaseManager::getInstance();
        string query = "SELECT shop_id, unique_id, shop_name, location, shop_phone FROM shops ORDER BY shop_id ASC;";
        auto result = db.executeQuery(query);

        if (!result) {
            cout << "  [Error] Failed to fetch shops: " << result.error() << "\n";
        } else {
            sql::ResultSet* rs = result.value();
            bool hasShops = false;
            while (rs->next()) {
                hasShops = true;
                string idStr = rs->getString("unique_id");
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

            print("  Enter Shop ID to manage (or 0 to cancel/back): ");
            string shopIdInput;
            getline(cin, shopIdInput);
            if (shopIdInput.empty() || shopIdInput == "0") {
                return;
            }

            auto& db = db::DatabaseManager::getInstance();
            string query = "SELECT shop_id, unique_id, shop_name, location, shop_phone FROM shops WHERE unique_id = '" + shopIdInput + "' OR shop_id = '" + shopIdInput + "';";
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

            int shopId = crs->getInt("shop_id");
            string shopUniqueId = crs->getString("unique_id");
            string shopName = crs->getString("shop_name");
            string location = crs->getString("location");
            string phone = crs->getString("shop_phone");
            delete crs;

            println("\n  Managing Shop: {} (ID: {})", shopName, shopUniqueId);
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

                    string updateQuery = "UPDATE shops SET shop_name = '" + newName + "' WHERE shop_id = " + to_string(shopId) + ";";
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

                    string updateQuery = "UPDATE shops SET location = '" + newLoc + "' WHERE shop_id = " + to_string(shopId) + ";";
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

                    string updateQuery = "UPDATE shops SET shop_phone = '" + newPhone + "' WHERE shop_id = " + to_string(shopId) + ";";
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

        print("  Enter Shop ID to view inventory (or 0 to cancel): ");
        string shopIdInput;
        getline(cin, shopIdInput);
        if (shopIdInput.empty() || shopIdInput == "0") {
            return;
        }

        // Verify shop existence first
        auto& db = db::DatabaseManager::getInstance();
        string checkQuery = "SELECT shop_id, unique_id, shop_name FROM shops WHERE unique_id = '" + shopIdInput + "' OR shop_id = '" + shopIdInput + "';";
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
        int shopId = crs->getInt("shop_id");
        string shopUniqueId = crs->getString("unique_id");
        string shopName = crs->getString("shop_name");
        delete crs;

        println("\n  Inventory for: {} (ID: {})\n", shopName, shopUniqueId);

        vector<int> colWidths = {12, 29, 12, 12};
        tool::ui::printRow(colWidths, {"CAT ID", "ITEM NAME", "QUANTITY", "CONDITION"});
        tool::helper::drawLine(65, '-');

        string query = 
            "SELECT c.catalog_id, c.unique_id, c.name, COUNT(i.item_id) AS quantity, i.condition_status "
            "FROM apparel_catalog c "
            "JOIN apparel_item i ON c.catalog_id = i.catalog_id "
            "WHERE c.shop_id = " + to_string(shopId) + " AND i.is_deleted = 0 AND c.is_deleted = 0 "
            "GROUP BY c.catalog_id, c.unique_id, c.name, i.condition_status "
            "ORDER BY c.catalog_id ASC;";

        auto result = db.executeQuery(query);
        if (!result) {
            cout << "  [Error] Failed to fetch inventory: " << result.error() << "\n";
        } else {
            sql::ResultSet* rs = result.value();
            bool hasItems = false;
            while (rs->next()) {
                hasItems = true;
                string catIdStr = rs->getString("unique_id");
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

        tool::ui::pressZeroToReturn("dashboard", 65);
    }

    void showShopManagementSubmenu(const auth::UserSession& session) {
        bool inSubmenu = true;
        int invalidAttempts = 0;
        while (inSubmenu) {
            tool::ui::showHeader("SHOP & INVENTORY MANAGEMENT", 64);
            println("  [1] Register New Shop");
            println("  [2] Display & Manage Shop Information");
            println("  [3] View Shop/Branch Inventory");
            println("  [0] Return to Dashboard");
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");

            int choice;
            if (tool::input::readInt(choice)) {
                switch (choice) {
                    case 1:
                        invalidAttempts = 0;
                        registerNewShop();
                        break;
                    case 2:
                        invalidAttempts = 0;
                        manageShopInformation();
                        break;
                    case 3:
                        invalidAttempts = 0;
                        viewShopInventory();
                        break;
                    case 0:
                        invalidAttempts = 0;
                        inSubmenu = false;
                        break;
                    default:
                        println("  Invalid option.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                }
            } else {
                println("  Invalid selection.");
                if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            }
        }
    }

}