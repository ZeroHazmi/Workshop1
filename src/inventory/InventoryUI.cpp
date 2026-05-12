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
  bool inCatalog = true;
  string searchTerm = "";
  int currentPage = 1;
  const int itemsPerPage = 25;

  while (inCatalog) {
    tool::helper::clearScreen();
    tool::ui::displayTitle("APPAREL CATALOG", 75);
    println("");

    vector<int> colWidths = {4, 25, 12, 12, 10};

    // Header Row
    tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "CATEGORY", "PRICE/DAY", "AVAILABLE"});
    tool::helper::drawLine(75, '-');

    // Data Rows (Fetch from Database)
    auto result = inventory::apparel::getCatalogDisplay(searchTerm);
    if (result) {
      const auto& allItems = result.value();
      int totalItems = allItems.size();
      int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;
      
      // Bound checking for currentPage
      if (currentPage > totalPages) currentPage = totalPages;
      if (currentPage < 1) currentPage = 1;

      int startIndex = (currentPage - 1) * itemsPerPage;
      int endIndex = std::min(startIndex + itemsPerPage, totalItems);

      for (int i = startIndex; i < endIndex; ++i) {
        const auto &item = allItems[i];
        tool::ui::printRow(colWidths,
                           {to_string(item.catalog_id), item.description,
                            item.category, format("RM {:.2f}", item.daily_rate), to_string(item.available_stock)});
      }

      tool::helper::drawLine(75, '-');
      println("  Page {} of {} | Total Items: {}", currentPage, totalPages, totalItems);
    } else {
      println("  Error fetching catalog: {}", result.error());
    }

    tool::helper::drawLine(75, '=');
    println("");
    
    print("  [n] Next, [p] Prev, [0] Exit, or type to search: ");
    
    string input;
    getline(cin, input);

    if (input == "0") {
      inCatalog = false;
    } else if (input == "n" || input == "N") {
      currentPage++;
    } else if (input == "p" || input == "P") {
      currentPage--;
    } else {
      searchTerm = input;
      currentPage = 1;
    }
  }
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

  print("  Apparel Name/Description (or '0' to cancel): ");
  getline(cin, description);
  if (description == "0" || description == "cancel") return;

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

  ApparelCatalog newCatalog;
  newCatalog.shop_id = shop_id;
  newCatalog.description = description;
  newCatalog.category = category;
  newCatalog.daily_rate = daily_rate;
  newCatalog.size = size;
  newCatalog.colour = colour;

  auto insertResult = inventory::apparel::addApparelCatalog(newCatalog, total_stock, condition_status);
  if (insertResult) {
    println("\n  Apparel registered successfully with {} items created!", total_stock);
  } else {
    println("\n  Failed to register apparel: {}", insertResult.error());
  }

  println("\nPress Enter to return to dashboard...");
  cin.ignore(10000, '\n');
}
} // namespace inventory::ui