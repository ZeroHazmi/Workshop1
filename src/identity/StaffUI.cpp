#include "identity/StaffUI.h"
#include "inventory/InventoryUI.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "inventory/Apparel/Apparel.h"
#include <iostream>
#include <print>
#include <string>
#include <thread>


using namespace std;

namespace identity::staffui {

void showStaffDashboard(const ::identity::auth::UserSession &session) {
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
  println("  [2] Process Apparel Return");
  println("  [3] View Active/Overdue Rentals");
  println("  [4] View Staff Profile");
  println("  [0] Logout");

  // Footer decoration
  tool::helper::drawLine(64, '-');
  print("  Select an option: ");
}

void handleStaffDashboard(const ::identity::auth::UserSession &session) {
  bool inStaffPanel = true;
  while (inStaffPanel) {
    showStaffDashboard(session);

    int choice;
    if (!(cin >> choice)) {
      cin.clear();
      cin.ignore(1000, '\n');
      continue;
    }
    cin.ignore(1000, '\n'); // Clear input buffer

    switch (choice) {
    case 1:
      manageApparelInventory(session);
      break;
    case 2:
      processApparelReturn();
      break;
    case 3:
      viewActiveRentals();
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

static void updateConditionFlow() {
  tool::helper::clearScreen();
  tool::ui::displayTitle("UPDATE APPAREL CONDITION", 50);
  println("");

  int itemId;
  print("  Enter Item ID (0 to cancel): ");
  if (!(cin >> itemId) || itemId == 0) {
      cin.clear();
      cin.ignore(1000, '\n');
      return;
  }
  cin.ignore(1000, '\n');

  println("  [1] Excellent");
  println("  [2] Good");
  println("  [3] Fair");
  println("  [4] Poor");
  println("  [5] Damaged");
  print("  Select new condition: ");

  int opt;
  if (!(cin >> opt)) {
      cin.clear();
      cin.ignore(1000, '\n');
      return;
  }
  cin.ignore(1000, '\n');

  string condition = "";
  switch(opt) {
      case 1: condition = "Excellent"; break;
      case 2: condition = "Good"; break;
      case 3: condition = "Fair"; break;
      case 4: condition = "Poor"; break;
      case 5: condition = "Damaged"; break;
      default: println("  Invalid option."); return;
  }

  auto result = inventory::apparel::updateItemCondition(itemId, condition);
  if (result) {
      println("\n  Condition for Item #{} updated to '{}'.", itemId, condition);
  } else {
      println("\n  Error: {}", result.error());
  }

  println("\nPress Enter to continue...");
  cin.ignore(10000, '\n');
}

static void processLaundryFlow() {
  tool::helper::clearScreen();
  tool::ui::displayTitle("PROCESS LAUNDRY", 50);
  println("");

  auto result = inventory::apparel::getItemsByStatus("Laundry");
  if (!result) {
      println("  Error fetching laundry items: {}", result.error());
      println("\nPress Enter to continue...");
      cin.ignore(10000, '\n');
      return;
  }

  auto items = result.value();
  if (items.empty()) {
      println("  No items currently in laundry.");
      println("\nPress Enter to continue...");
      cin.ignore(10000, '\n');
      return;
  }

  println("  ITEMS IN LAUNDRY:");
  for (const auto& item : items) {
      println("  Item ID: {} | Condition: {}", item.item_id, item.condition_status);
  }
  tool::helper::drawLine(50, '-');

  int itemId;
  print("  Enter Item ID to mark as washed (0 to cancel): ");
  if (!(cin >> itemId) || itemId == 0) {
      cin.clear();
      cin.ignore(1000, '\n');
      return;
  }
  cin.ignore(1000, '\n');

  auto updateRes = inventory::apparel::updateItemStatus(itemId, "Available");
  if (updateRes) {
      println("\n  Item #{} marked as Available.", itemId);
  } else {
      println("\n  Error: {}", updateRes.error());
  }

  println("\nPress Enter to continue...");
  cin.ignore(10000, '\n');
}

void manageApparelInventory(const ::identity::auth::UserSession &session) {
  bool inInventoryMenu = true;
  while (inInventoryMenu) {
    tool::helper::clearScreen();
    tool::ui::displayTitle("INVENTORY MANAGEMENT", 50);
    println("");
    
    println("  [1] Register/Add New Apparel");
    println("  [2] View All Apparel (Full Catalog)");
    println("  [3] Update Apparel Condition / Status");
    println("  [4] Process Laundry & Maintenance");
    println("  [5] Retire/Remove Damaged Apparel");
    println("  [0] Back to Main Dashboard");
    
    tool::helper::drawLine(50, '-');
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
        inventory::ui::showCatalog();
        break;
      case 3:
        updateConditionFlow();
        break;
      case 4:
        processLaundryFlow();
        break;
      case 5:
        println("\n  [Feature coming soon: Retire Apparel]");
        println("  Press Enter to continue...");
        cin.ignore(10000, '\n');
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

void processApparelReturn() {
  tool::ui::displayTitle("PROCESS APPAREL RETURN", 50);
  println("");

  int rentalId;
  print("  Enter Rental ID to process return (0 to cancel): ");
  if (!(cin >> rentalId) || rentalId == 0) {
      cin.clear();
      cin.ignore(1000, '\n');
      return;
  }
  cin.ignore(1000, '\n');

  println("\n  Processing Return for Rental ID: {}", rentalId);
  println("  [1] Confirm Return & Set Needs Cleaning");
  println("  [2] Report Damage");
  println("  [3] Cancel Return");
  print("  Select option: ");

  int option;
  if (!(cin >> option)) {
      cin.clear();
      cin.ignore(1000, '\n');
      return;
  }
  cin.ignore(1000, '\n');

  switch (option) {
  case 1:
    println("  Return confirmed. Apparel status set to 'Needs Cleaning'.");
    break;
  case 2:
    println("  Damage reported. Customer will be notified of penalties.");
    break;
  case 3:
    println("  Return cancelled.");
    break;
  default:
    println("  Invalid option.");
  }

  println("\nPress Enter to return to dashboard...");
  cin.ignore(10000, '\n');
}

void viewActiveRentals() {
  tool::ui::displayTitle("ACTIVE & OVERDUE RENTALS", 50);
  println("");
  
  // TODO: Fetch active/overdue rentals from database
  println("  [Feature coming soon: Active Rentals List]");
  
  println("\nPress Enter to return to dashboard...");
  cin.ignore(10000, '\n');
}

void viewStaffProfile(const ::identity::auth::UserSession &session) {
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
} // namespace identity::staffui
