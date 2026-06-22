#include "DatabaseManager/DatabaseManager.h"
#include "identity/AdminUI.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "tool/input.h"
#include <cppconn/resultset.h>
#include <format>
#include <iostream>
#include <optional>
#include <print>
#include <string>
#include <thread>
#include <vector>

namespace auth = ::identity::auth;
namespace db = ::database;

using namespace std;

namespace identity::adminui {

// =========================================================================
// Internal data carrier — returned by lookupStaffRecord()
// =========================================================================

struct StaffRecord {
  int staffId;
  int userId;
  string uniqueId;
  string name;
  string position;
  string phone;
  string shopName;
};

// =========================================================================
// Register Staff Account
// =========================================================================

void registerStaffAccount() {
  tool::ui::showHeader("REGISTER STAFF ACCOUNT", 64);

  // Use the new registration flow with staff role
  auth::Auth::handleRegisterFlow("staff");
}

// =========================================================================
// Sub-action: Update Staff Information
// =========================================================================

static void updateStaffInformation(const StaffRecord &staff) {
  println("\n  --- UPDATE STAFF INFORMATION ---");

  string newName, newPosition, newPhone;
  print("  Enter New Name (leave blank to keep current): ");
  getline(cin, newName);
  print("  Enter New Position (leave blank to keep current): ");
  getline(cin, newPosition);
  print("  Enter New Phone (leave blank to keep current): ");
  getline(cin, newPhone);

  // Fall back to the existing value when a field is left blank
  if (newName.empty())
    newName = staff.name;
  if (newPosition.empty())
    newPosition = staff.position;
  if (newPhone.empty())
    newPhone = staff.phone;

  auto &db = db::DatabaseManager::getInstance();
  string updateQuery = "UPDATE STAFF SET staff_name = '" + newName +
                       "', position = '" + newPosition + "', phone_no = '" +
                       newPhone +
                       "' WHERE staff_id = " + to_string(staff.staffId) + ";";

  auto updateRes = db.executeUpdate(updateQuery);
  if (updateRes) {
    println("\n  Success: Staff information updated successfully.");
  } else {
    cout << "  [Error] Failed to update staff: " << updateRes.error() << "\n";
  }
}

// =========================================================================
// Sub-action: Change Shop Assignment
// =========================================================================

static void changeShopAssignment(const StaffRecord &staff) {
  println("\n  --- CHANGE SHOP ASSIGNMENT ---");

  print("  Enter New Shop ID to assign: ");
  string newShopIdInput;
  getline(cin, newShopIdInput);
  if (newShopIdInput.empty()) {
    println("  Invalid input. Operation cancelled.");
    return;
  }

  // Resolve shop unique ID to internal integer shop_id
  auto &db = db::DatabaseManager::getInstance();
  string shopQuery = "SELECT shop_id FROM shops "
                     "WHERE unique_id = '" +
                     newShopIdInput + "';";

  auto shopRes = db.executeQuery(shopQuery);
  int resolvedShopId = 0;
  if (shopRes) {
    sql::ResultSet *srs = shopRes.value();
    if (srs->next()) {
      resolvedShopId = srs->getInt("shop_id");
    }
    delete srs;
  }

  if (resolvedShopId == 0) {
    println("  [Error] Shop ID not found. Assignment failed.");
    return;
  }

  string updateQuery =
      "UPDATE STAFF SET shop_id = " + to_string(resolvedShopId) +
      " WHERE staff_id = " + to_string(staff.staffId) + ";";

  auto updateRes = db.executeUpdate(updateQuery);
  if (updateRes) {
    println("\n  Success: Shop assignment updated successfully.");
  } else {
    cout << "  [Error] Failed to update assignment: " << updateRes.error()
         << "\n";
  }
}

// =========================================================================
// Sub-action: Terminate (Soft-Delete) Staff Account
// =========================================================================

static void terminateStaffAccount(const StaffRecord &staff) {
  println("\n  --- TERMINATE STAFF ACCOUNT ---");

  print("  Are you sure you want to soft delete this staff account? (Y/N): ");
  string confirm;
  getline(cin, confirm);
  if (confirm != "Y" && confirm != "y") {
    println("  Termination cancelled.");
    return;
  }

  auto &db = db::DatabaseManager::getInstance();

  // 1. Soft delete the STAFF record
  string deleteStaffQuery =
      "UPDATE STAFF SET is_deleted = 1 WHERE staff_id = " +
      to_string(staff.staffId) + ";";
  auto resStaff = db.executeUpdate(deleteStaffQuery);

  // 2. Soft delete the linked USERS login record
  string deleteUserQuery = "UPDATE USERS SET is_deleted = 1 WHERE user_id = " +
                           to_string(staff.userId) + ";";
  auto resUser = db.executeUpdate(deleteUserQuery);

  if (resStaff && resUser) {
    println("\n  Success: Staff account and linked user login soft deleted "
            "successfully.");
  } else {
    cout << "  [Warning] Partial failure during deletion.\n";
  }
}

// =========================================================================
// Helper: Display the full staff list table
// =========================================================================

static void displayStaffList() {
  auto &db = db::DatabaseManager::getInstance();
  string query =
      "SELECT s.staff_id, s.unique_id, s.user_id, s.staff_name, u.username, "
      "s.position, s.phone_no, s.shop_id, "
      "       COALESCE(sh.shop_name, CONCAT('Shop #', s.shop_id)) AS shop_name "
      "FROM STAFF s "
      "JOIN USERS u ON s.user_id = u.user_id "
      "LEFT JOIN shops sh ON s.shop_id = sh.shop_id "
      "WHERE s.is_deleted = 0 "
      "ORDER BY s.staff_id ASC;";

  auto result = db.executeQuery(query);
  if (!result) {
    cout << "  [Error] Failed to fetch staff list: " << result.error() << "\n";
    return;
  }

  sql::ResultSet *rs = result.value();
  vector<int> colWidths = {12, 16, 12, 12, 12, 14};

  tool::ui::printRow(colWidths, {"ID", "STAFF NAME", "USERNAME", "POSITION",
                                 "PHONE NO", "SHOP ASSIGNED"});
  tool::helper::drawLine(78, '-');

  bool hasStaff = false;
  while (rs->next()) {
    hasStaff = true;
    tool::ui::printRow(colWidths,
                       {rs->getString("unique_id"), rs->getString("staff_name"),
                        rs->getString("username"), rs->getString("position"),
                        rs->getString("phone_no"), rs->getString("shop_name")});
  }
  delete rs;

  if (!hasStaff) {
    println("  No staff accounts registered in the database.");
  }
  tool::helper::drawLine(78, '=');
  println("");
}

// =========================================================================
// Helper: Resolve a staff ID string to a fully populated StaffRecord
// =========================================================================

static optional<StaffRecord> lookupStaffRecord(const string &staffIdInput) {
  auto &db = db::DatabaseManager::getInstance();

  string checkQuery =
      "SELECT s.staff_id, s.unique_id, s.user_id, s.staff_name, s.position, "
      "s.phone_no, s.shop_id, "
      "       COALESCE(sh.shop_name, CONCAT('Shop #', s.shop_id)) AS shop_name "
      "FROM STAFF s "
      "LEFT JOIN shops sh ON s.shop_id = sh.shop_id "
      "WHERE s.unique_id = '" +
      staffIdInput + "' AND s.is_deleted = 0;";

  auto checkRes = db.executeQuery(checkQuery);
  if (!checkRes) {
    cout << "  [Error] Database error: " << checkRes.error() << "\n";
    return nullopt;
  }

  sql::ResultSet *crs = checkRes.value();
  if (!crs->next()) {
    delete crs;
    println("  [Error] Staff ID not found.");
    this_thread::sleep_for(chrono::milliseconds(1500));
    return nullopt;
  }

  StaffRecord record{crs->getInt("staff_id"),     crs->getInt("user_id"),
                     crs->getString("unique_id"), crs->getString("staff_name"),
                     crs->getString("position"),  crs->getString("phone_no"),
                     crs->getString("shop_name")};
  delete crs;
  return record;
}

// =========================================================================
// Manage Staff Account  (pure routing orchestrator)
// =========================================================================

void manageStaffAccount() {
  bool managing = true;
  while (managing) {
    tool::ui::showHeader("MANAGE STAFF ACCOUNT", 78);

    displayStaffList();

    print("  Enter Staff ID to manage (or 0 to cancel): ");
    string staffIdInput;
    getline(cin, staffIdInput);
    if (staffIdInput.empty() || staffIdInput == "0") {
      managing = false;
      return;
    }

    auto staffOpt = lookupStaffRecord(staffIdInput);
    if (!staffOpt) {
      continue;
    }
    StaffRecord staff = staffOpt.value();

    bool editingStaff = true;
    while (editingStaff) {
      tool::helper::clearScreen();
      println("\n  Managing Staff: {} (ID: {})", staff.name, staff.uniqueId);
      println("  Current Position: {}", staff.position);
      println("  Assigned Branch : {}", staff.shopName);
      tool::helper::drawLine(74, '-');

      println("\n  [1] Update Staff Information");
      println("  [2] Change Shop Assignment");
      println("  [3] Remove / Terminate Staff Account (Soft Delete)");
      println("  [0] Back to Staff List");
      print("  Select option: ");

      int option;
      if (!(cin >> option)) {
        cin.clear();
        cin.ignore(1000, '\n');
        println("  Invalid input. Please try again.");
        this_thread::sleep_for(chrono::milliseconds(1000));
        continue;
      }
      cin.ignore(1000, '\n');

      if (option == 0) {
        editingStaff = false;
        break;
      }

      switch (option) {
      case 1: {
        updateStaffInformation(staff);
        auto updatedOpt = lookupStaffRecord(staff.uniqueId);
        if (updatedOpt) {
          staff = updatedOpt.value();
        }
        this_thread::sleep_for(chrono::milliseconds(1500));
        break;
      }
      case 2: {
        changeShopAssignment(staff);
        auto updatedOpt = lookupStaffRecord(staff.uniqueId);
        if (updatedOpt) {
          staff = updatedOpt.value();
        }
        this_thread::sleep_for(chrono::milliseconds(1500));
        break;
      }
      case 3:
        terminateStaffAccount(staff);
        this_thread::sleep_for(chrono::milliseconds(1500));
        editingStaff = false;
        break;
      default:
        println("  Invalid option.");
        this_thread::sleep_for(chrono::milliseconds(1000));
      }
    }
  }
}

// =========================================================================
// Staff Management Submenu
// =========================================================================

void showStaffManagementSubmenu(const auth::UserSession &session) {
  bool inSubmenu = true;
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
        registerStaffAccount();
        break;
      case 2:
        manageStaffAccount();
        break;
      case 0:
        inSubmenu = false;
        break;
      default:
        println("  Invalid option.");
        this_thread::sleep_for(chrono::milliseconds(1000));
      }
    } else {
      println("  Invalid selection.");
      this_thread::sleep_for(chrono::milliseconds(1000));
    }
  }
}

} // namespace identity::adminui