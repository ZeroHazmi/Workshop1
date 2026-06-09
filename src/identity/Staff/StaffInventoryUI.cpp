#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include "inventory/Apparel/Apparel.h"
#include "inventory/InventoryUI.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <print>
#include <string>
#include <iostream>
#include <vector>
#include <thread>
#include <format>

using namespace std;

namespace identity::staffui {

    static bool displayCatalogItemsHelper(const string& catalogUniqueId, const string& filter = "") {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string filterClause = "";
        if (!filter.empty()) {
            filterClause = std::format(" AND (name LIKE '%{}%' OR category LIKE '%{}%')", filter, filter);
        }

        // Fetch the catalog name to make the header nice (supports ID, exact name, or partial name match, scoped to search results if filter is active)
        std::string nameQuery = std::format(
            "SELECT catalog_id, name, unique_id FROM apparel_catalog "
            "WHERE (unique_id = '{}' OR catalog_id = '{}' OR name = '{}' OR name LIKE '%{}%'){} AND is_deleted = 0 LIMIT 1",
            catalogUniqueId, catalogUniqueId, catalogUniqueId, catalogUniqueId, filterClause
        );
        auto nameRes = db.executeQuery(nameQuery);
        if (!nameRes) return false;
        
        sql::ResultSet* nrs = nameRes.value();
        if (!nrs->next()) {
            delete nrs;
            std::print("  [Error] Catalog ID/Name '{}' not found.\n", catalogUniqueId);
            this_thread::sleep_for(chrono::milliseconds(1000));
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
            this_thread::sleep_for(chrono::milliseconds(1000));
        }
        return found;
    }

    static void updateConditionFlow() {
        int itemId = -1;
        string itemUniqueId = "";
        bool firstLoad = true;
        while (true) {
            if (firstLoad) {
                tool::ui::showHeader("UPDATE APPAREL CONDITION", 64);
                println("");
                firstLoad = false;
            }
            print("  Enter Item ID to update (or 'S' to search by Catalog, '0' to cancel): ");
            string input;
            getline(cin, input);
            
            if (input == "0") return;
            if (input == "S" || input == "s") {
                print("  Enter catalog search query (leave blank for all): ");
                string qText;
                getline(cin, qText);

                auto& db = database::DatabaseManager::getInstance();
                std::string catQuery = std::format(
                    "SELECT unique_id, name, category, daily_rate FROM apparel_catalog "
                    "WHERE is_deleted = 0 AND (name LIKE '%{}%' OR category LIKE '%{}%') "
                    "ORDER BY catalog_id ASC LIMIT 10",
                    qText, qText
                );
                auto catRes = db.executeQuery(catQuery);
                if (catRes) {
                    sql::ResultSet* crs = catRes.value();
                    bool hasResults = false;
                    vector<int> colWidths = {12, 25, 15, 10};
                    while (crs->next()) {
                        if (!hasResults) {
                            println("\n  AVAILABLE APPAREL CATALOGS (Showing top 10 matches):");
                            tool::ui::printRow(colWidths, {"CATALOG ID", "NAME", "CATEGORY", "RATE/DAY"});
                            tool::helper::drawLine(64, '-');
                            hasResults = true;
                        }
                        tool::ui::printRow(colWidths, {
                            crs->getString("unique_id"),
                            crs->getString("name"),
                            crs->getString("category"),
                            std::format("RM {:.2f}", crs->getDouble("daily_rate"))
                        });
                    }
                    delete crs;
                    if (hasResults) {
                        tool::helper::drawLine(64, '=');
                        println("");
                    } else {
                        println("\n  No matching catalogs found.");
                        println("");
                    }
                }

                print("  Enter Catalog ID or Name to view physical items (or 0 to cancel): ");
                string catalogInput;
                getline(cin, catalogInput);
                if (catalogInput == "0" || catalogInput.empty()) {
                    continue;
                }
                bool foundCat = displayCatalogItemsHelper(catalogInput, qText);
                println("");
                if (!foundCat) {
                    tool::helper::clearScreen();
                    firstLoad = true;
                }
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
                this_thread::sleep_for(chrono::milliseconds(1000));
                tool::helper::clearScreen();
                firstLoad = true;
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

        tool::ui::pressZeroToReturn("previous menu", 64);
    }

    static void retireApparelFlow() {
        int itemId = -1;
        string itemUniqueId = "";
        bool firstLoad = true;
        auto& db = database::DatabaseManager::getInstance();
        
        while (true) {
            if (firstLoad) {
                tool::ui::showHeader("RETIRE / REMOVE DAMAGED APPAREL", 64);
                println("");
 
                std::string listQuery = 
                    "SELECT i.unique_id, c.name AS item_name, i.size, i.status "
                    "FROM apparel_item i "
                    "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
                    "WHERE i.condition_status = 'Damaged' AND i.is_deleted = 0 "
                    "ORDER BY i.item_id ASC";
 
                auto listRes = db.executeQuery(listQuery);
                if (listRes) {
                    sql::ResultSet* rs = listRes.value();
                    std::vector<std::vector<std::string>> rows;
                    while (rs->next()) {
                        rows.push_back({
                            rs->getString("unique_id"),
                            rs->getString("item_name"),
                            rs->getString("size"),
                            rs->getString("status")
                        });
                    }
                    delete rs;
 
                    if (rows.empty()) {
                        println("  No physically damaged items currently registered in active stock.");
                        println("");
                    } else {
                        println("  ACTIVE DAMAGED ITEMS IN INVENTORY:");
                        vector<int> colWidths = {12, 22, 10, 12};
                        tool::ui::printRow(colWidths, {"ITEM ID", "ITEM NAME", "SIZE", "AVAIL. STATUS"});
                        tool::helper::drawLine(64, '-');
                        for (const auto& row : rows) {
                            tool::ui::printRow(colWidths, row);
                        }
                        tool::helper::drawLine(64, '=');
                        println("");
                    }
                }
                firstLoad = false;
            }
 
            print("  Enter Item ID to retire (or 'S' to search by Catalog, '0' to cancel): ");
            string input;
            getline(cin, input);
            
            if (input == "0") return;
            if (input == "S" || input == "s") {
                print("  Enter catalog search query (leave blank for all): ");
                string qText;
                getline(cin, qText);

                std::string catQuery = std::format(
                    "SELECT unique_id, name, category, daily_rate FROM apparel_catalog "
                    "WHERE is_deleted = 0 AND (name LIKE '%{}%' OR category LIKE '%{}%') "
                    "ORDER BY catalog_id ASC LIMIT 10",
                    qText, qText
                );
                auto catRes = db.executeQuery(catQuery);
                if (catRes) {
                    sql::ResultSet* crs = catRes.value();
                    bool hasResults = false;
                    vector<int> colWidths = {12, 25, 15, 10};
                    while (crs->next()) {
                        if (!hasResults) {
                            println("\n  AVAILABLE APPAREL CATALOGS (Showing top 10 matches):");
                            tool::ui::printRow(colWidths, {"CATALOG ID", "NAME", "CATEGORY", "RATE/DAY"});
                            tool::helper::drawLine(64, '-');
                            hasResults = true;
                        }
                        tool::ui::printRow(colWidths, {
                            crs->getString("unique_id"),
                            crs->getString("name"),
                            crs->getString("category"),
                            std::format("RM {:.2f}", crs->getDouble("daily_rate"))
                        });
                    }
                    delete crs;
                    if (hasResults) {
                        tool::helper::drawLine(64, '=');
                        println("");
                    } else {
                        println("\n  No matching catalogs found.");
                        println("");
                    }
                }

                print("  Enter Catalog ID or Name to view physical items (or 0 to cancel): ");
                string catalogInput;
                getline(cin, catalogInput);
                if (catalogInput == "0" || catalogInput.empty()) {
                    continue;
                }
                bool foundCat = displayCatalogItemsHelper(catalogInput, qText);
                println("");
                if (!foundCat) {
                    tool::helper::clearScreen();
                    firstLoad = true;
                }
                continue;
            }
            
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
                this_thread::sleep_for(chrono::milliseconds(1000));
                tool::helper::clearScreen();
                firstLoad = true;
            }
        }

        // Fetch details of the item to confirm retirement
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
            tool::ui::pressZeroToReturn("previous menu", 64);
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

        tool::ui::pressZeroToReturn("previous menu", 64);
    }

    static void processLaundryFlow() {
        tool::ui::showHeader("PROCESS LAUNDRY", 64);
        println("");

        auto& db = database::DatabaseManager::getInstance();
        std::string query = 
            "SELECT i.item_id, i.unique_id, c.name AS item_name, i.size, i.condition_status "
            "FROM apparel_item i "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "WHERE i.status = 'Laundry' AND i.is_deleted = 0 AND c.is_deleted = 0 "
            "ORDER BY i.item_id ASC";

        auto result = db.executeQuery(query);
        if (!result) {
            println("  Error fetching laundry items: {}", result.error());
            tool::ui::pressZeroToReturn("previous menu", 64);
            return;
        }

        sql::ResultSet* rs = result.value();
        std::vector<std::vector<std::string>> rows;
        while (rs->next()) {
            rows.push_back({
                rs->getString("unique_id"),
                rs->getString("item_name"),
                rs->getString("size"),
                rs->getString("condition_status")
            });
        }
        delete rs;

        if (rows.empty()) {
            println("  No items currently in laundry.");
            tool::ui::pressZeroToReturn("previous menu", 64);
            return;
        }

        vector<int> colWidths = {12, 22, 10, 12};
        tool::ui::printRow(colWidths, {"ITEM ID", "ITEM NAME", "SIZE", "CONDITION"});
        tool::helper::drawLine(64, '-');

        for (const auto& row : rows) {
            tool::ui::printRow(colWidths, row);
        }
        tool::helper::drawLine(64, '=');
        println("");

        print("  Enter Item ID to mark as washed (0 to cancel): ");
        string laundryInput;
        getline(cin, laundryInput);
        if (laundryInput.empty() || laundryInput == "0") {
            return;
        }

        // Resolve item unique ID
        int itemId = -1;
        string itemUniqueId = "";
        std::string q = std::format("SELECT item_id, unique_id FROM apparel_item WHERE (unique_id = '{}' OR item_id = '{}') AND is_deleted = 0", laundryInput, laundryInput);
        auto r = db.executeQuery(q);
        if (r) {
            sql::ResultSet* rrs = r.value();
            if (rrs->next()) {
                itemId = rrs->getInt("item_id");
                itemUniqueId = rrs->getString("unique_id");
            }
            delete rrs;
        }

        if (itemId == -1) {
            println("  [Error] Item ID not found.");
            tool::ui::pressZeroToReturn("previous menu", 64);
            return;
        }

        auto updateRes = inventory::apparel::updateItemStatus(itemId, "Available");
        if (updateRes) {
            println("\n  Item #{} marked as Available.", itemUniqueId);
        } else {
            println("\n  Error: {}", updateRes.error());
        }

        tool::ui::pressZeroToReturn("previous menu", 64);
    }

    static void viewRetiredItemsFlow() {
        tool::ui::showHeader("RETIRED / WRITTEN-OFF ITEMS", 64);
        println("");

        auto& db = database::DatabaseManager::getInstance();
        std::string query = 
            "SELECT i.unique_id, c.name AS item_name, i.size, i.condition_status "
            "FROM apparel_item i "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "WHERE (i.status = 'Retired' OR i.is_deleted = 1) "
            "ORDER BY i.item_id DESC";

        auto result = db.executeQuery(query);
        if (!result) {
            println("  Error fetching retired items: {}", result.error());
            tool::ui::pressZeroToReturn("previous menu", 64);
            return;
        }

        sql::ResultSet* rs = result.value();
        std::vector<std::vector<std::string>> rows;
        while (rs->next()) {
            rows.push_back({
                rs->getString("unique_id"),
                rs->getString("item_name"),
                rs->getString("size"),
                rs->getString("condition_status")
            });
        }
        delete rs;

        if (rows.empty()) {
            println("  No retired or written-off items found in the records.");
            tool::ui::pressZeroToReturn("previous menu", 64);
            return;
        }

        vector<int> colWidths = {12, 22, 10, 12};
        tool::ui::printRow(colWidths, {"ITEM ID", "ITEM NAME", "SIZE", "RET. CONDITION"});
        tool::helper::drawLine(64, '-');

        for (const auto& row : rows) {
            tool::ui::printRow(colWidths, row);
        }
        tool::helper::drawLine(64, '=');
        println("");

        tool::ui::pressZeroToReturn("previous menu", 64);
    }

    void manageApparelInventory(const ::identity::auth::UserSession& session) {
        bool inInventoryMenu = true;
        int invalidAttempts = 0;
        while (inInventoryMenu) {
            tool::ui::showHeader("INVENTORY MANAGEMENT", 64);
            
            println("  [1] Register/Add New Apparel");
            println("  [2] View All Apparel (Full Catalog)");
            println("  [3] Update Apparel Condition / Status");
            println("  [4] Process Laundry & Maintenance");
            println("  [5] Retire/Remove Damaged Apparel");
            println("  [6] View Retired/Written-off Items");
            println("  [0] Back to Main Dashboard");
            
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");
            
            int choice;
            if (tool::input::readInt(choice)) {
                switch(choice) {
                    case 1:
                        invalidAttempts = 0;
                        inventory::ui::registerNewApparel(session);
                        break;
                    case 2:
                        invalidAttempts = 0;
                        inventory::ui::showCatalog(session);
                        break;
                    case 3:
                        invalidAttempts = 0;
                        updateConditionFlow();
                        break;
                    case 4:
                        invalidAttempts = 0;
                        processLaundryFlow();
                        break;
                    case 5:
                        invalidAttempts = 0;
                        retireApparelFlow();
                        break;
                    case 6:
                        invalidAttempts = 0;
                        viewRetiredItemsFlow();
                        break;
                    case 0:
                        invalidAttempts = 0;
                        inInventoryMenu = false;
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
