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

using namespace std;

namespace identity::adminui {

    void registerStaffAccount() {
        tool::ui::showHeader("REGISTER STAFF ACCOUNT", 64);

        // Use the new registration flow with staff role
        ::identity::auth::Auth::handleRegisterFlow("staff");
        
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

    void manageStaffAccount() {
        tool::ui::showHeader("MANAGE STAFF ACCOUNT", 78);

        // 1. Fetch and display all registered staff in a formatted table
        auto& db = database::DatabaseManager::getInstance();
        std::string primaryQuery = 
            "SELECT s.staff_id, s.unique_id, s.user_id, s.staff_name, u.username, s.position, s.phone_no, s.shop_id, "
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
                "SELECT s.staff_id, s.unique_id, s.user_id, s.staff_name, u.username, s.position, s.phone_no, s.shop_id, "
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
            vector<int> colWidths = {12, 16, 12, 12, 12, 14};
            
            tool::ui::printRow(colWidths, {"ID", "STAFF NAME", "USERNAME", "POSITION", "PHONE NO", "SHOP ASSIGNED"});
            tool::helper::drawLine(78, '-');

            bool hasStaff = false;
            while (rs->next()) {
                hasStaff = true;
                string idStr = rs->getString("unique_id");
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

        print("  Enter Staff ID to manage (or 0 to cancel): ");
        string staffIdInput;
        getline(cin, staffIdInput);
        if (staffIdInput.empty() || staffIdInput == "0") {
            return;
        }

        // 2. Fetch specific staff info to verify existence and print details
        std::string checkQuery = "SELECT s.staff_id, s.unique_id, s.user_id, s.staff_name, s.position, s.phone_no, s.shop_id, "
                                 "       COALESCE(sh.shop_name, CONCAT('Shop #', s.shop_id)) AS shop_name "
                                 "FROM STAFF s "
                                 "LEFT JOIN shops sh ON s.shop_id = sh.shop_id "
                                 "WHERE (s.unique_id = '" + staffIdInput + "' OR s.staff_id = '" + staffIdInput + "') AND s.is_deleted = 0;";

        auto checkRes = db.executeQuery(checkQuery);
        if (!checkRes) {
            checkQuery = "SELECT staff_id, unique_id, user_id, staff_name, position, phone_no, shop_id, "
                         "       CONCAT('Shop #', shop_id) AS shop_name "
                         "FROM STAFF "
                         "WHERE (unique_id = '" + staffIdInput + "' OR staff_id = '" + staffIdInput + "') AND is_deleted = 0;";
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

        int staffId = crs->getInt("staff_id");
        string staffUniqueId = crs->getString("unique_id");
        string staffName = crs->getString("staff_name");
        string position = crs->getString("position");
        string phone = crs->getString("phone_no");
        string shopName = crs->getString("shop_name");
        int shopId = crs->getInt("shop_id");
        int userId = crs->getInt("user_id");
        delete crs;

        println("\n  Managing Staff: {} (ID: {})", staffName, staffUniqueId);
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
                print("  Enter New Shop ID to assign: ");
                string newShopIdInput;
                getline(cin, newShopIdInput);
                if (newShopIdInput.empty()) {
                    println("  Invalid input. Operation cancelled.");
                    break;
                }

                // Resolve shop's unique ID to internal integer shop_id
                std::string shopQuery = "SELECT shop_id FROM shops WHERE unique_id = '" + newShopIdInput + "' OR shop_id = '" + newShopIdInput + "';";
                auto shopRes = db.executeQuery(shopQuery);
                int resolvedShopId = 0;
                if (shopRes) {
                    sql::ResultSet* srs = shopRes.value();
                    if (srs->next()) {
                        resolvedShopId = srs->getInt("shop_id");
                    }
                    delete srs;
                }

                if (resolvedShopId == 0) {
                    println("  [Error] Shop ID not found. Assignment failed.");
                    break;
                }

                std::string updateQuery = "UPDATE STAFF SET shop_id = " + to_string(resolvedShopId) + 
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

        tool::ui::pressZeroToReturn("dashboard", 64);
    }

    void showStaffManagementSubmenu(const ::identity::auth::UserSession& session) {
        bool inSubmenu = true;
        int invalidAttempts = 0;
        while (inSubmenu) {
            tool::ui::showHeader("STAFF MANAGEMENT", 64);
            println("  [1] Register Staff Account");
            println("  [2] Manage Staff Account");
            println("  [0] Return to Dashboard");
            tool::helper::drawLine(64, '-');
            print("  Select an option: ");

            int choice;
            if (tool::input::readInt(choice)) {
                switch (choice) {
                    case 1:
                        invalidAttempts = 0;
                        registerStaffAccount();
                        break;
                    case 2:
                        invalidAttempts = 0;
                        manageStaffAccount();
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
