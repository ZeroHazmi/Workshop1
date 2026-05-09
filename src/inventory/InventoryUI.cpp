#include "inventory/InventoryUI.h"
#include "identity/Profile/Profile.h"
#include "inventory/Apparel/Apparel.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include <format>
#include <iostream>
#include <print>
#include <vector>


using namespace std;

namespace inventory::ui {

void showCatalog() {
  tool::helper::clearScreen();
  vector<int> colWidths = {4, 25, 12, 10};

  tool::ui::displayTitle("APPAREL CATALOG", 65);
  // Header Row
  tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "CATEGORY", "PRICE/DAY"});
  tool::helper::drawLine(65, '-');

  // Data Rows (Fetch from Database)
  auto result = inventory::apparel::getAllApparel();
  if (result) {
    for (const auto &item : result.value()) {
      tool::ui::printRow(colWidths,
                         {to_string(item.apparel_id), item.description,
                          item.category, format("RM {:.2f}", item.daily_rate)});
    }
  } else {
    println("  Error fetching catalog: {}", result.error());
  }

  tool::helper::drawLine(65, '=');
  println("\nPress Enter to return to dashboard...");
  cin.ignore(10000, '\n');
}

void registerNewApparel(const ::identity::auth::UserSession &session) {
  tool::ui::displayTitle("REGISTER NEW APPAREL", 50);
  println("");

  // Get Staff Profile to find shop_id
  auto staffProfileOpt =
      ::identity::profile::Profile::getStaffProfile(session.userid);
  if (!staffProfileOpt) {
    println("  Error: Could not retrieve staff profile. {}",
            staffProfileOpt.error());
    println("\nPress Enter to return to dashboard...");
    cin.ignore(10000, '\n');
    return;
  }

  int shop_id = staffProfileOpt.value().shop_id;

  string description, category, condition_status, size, colour;
  double daily_rate;
  int total_stock;

  print("  Apparel Name (Description): ");
  getline(cin, description);

  print("  Category: ");
  getline(cin, category);

  print("  Size (e.g. S, M, L, XL): ");
  getline(cin, size);

  print("  Colour: ");
  getline(cin, colour);

  print("  Total Stock Quantity: ");
  if (!(cin >> total_stock) || total_stock < 0) {
    cin.clear();
    cin.ignore(1000, '\n');
    println("  Invalid input for stock.");
    return;
  }

  print("  Rental Price (per day): RM ");
  if (!(cin >> daily_rate) || daily_rate < 0) {
    cin.clear();
    cin.ignore(1000, '\n');
    println("  Invalid input for price.");
    return;
  }

  cin.ignore(1000, '\n');
  print("  Condition Status (e.g. Excellent): ");
  getline(cin, condition_status);

  ApparelItem newItem;
  newItem.shop_id = shop_id;
  newItem.description = description;
  newItem.category = category;
  newItem.daily_rate = daily_rate;
  newItem.condition_status = condition_status;
  newItem.status = "Available"; // Default status
  newItem.size = size;
  newItem.colour = colour;
  newItem.total_stock = total_stock;
  newItem.available_stock = total_stock; // Set available_stock to same value as total_stock

  auto insertResult = inventory::apparel::addApparel(newItem);
  if (insertResult) {
    println("\n  Apparel registered successfully!");
  } else {
    println("\n  Failed to register apparel: {}", insertResult.error());
  }

  println("\nPress Enter to return to dashboard...");
  cin.ignore(10000, '\n');
}
} // namespace inventory::ui