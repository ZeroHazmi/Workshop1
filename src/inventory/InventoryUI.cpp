#include "inventory/InventoryUI.h"
#include "identity/Auth/Auth.h"
#include "transaction/Rental/Rental.h"
#include "tool/CLIComponents.h"
#include "tool/DateHelper.h"
#include <algorithm>
#include "tool/helper.h"
#include <inventory/Apparel/Apparel.h>
#include <chrono>
#include <iostream>
#include <print>
#include <string>
#include <thread>
#include <vector>


using namespace std;

namespace inventory::ui {

bool processRentalCheckout(const ::identity::auth::UserSession& session, const inventory::apparel::ApparelCatalog& item, int catalog_id) {
    bool renting = true;
    while (renting) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("RENT ITEM", 65);
        println("  Item: {} (ID: {})", item.name, item.catalog_id);
        println("");

        string selected_size, start, end, confirm;
        print("  Enter Size you wish to rent: ");
        getline(cin, selected_size);
        
        bool sizeAvailable = false;
        auto currentSizesOpt = inventory::apparel::getAvailableSizes(catalog_id);
        if (currentSizesOpt) {
            for (const auto& [size, qty] : currentSizesOpt.value()) {
                if (size == selected_size && qty > 0) {
                    sizeAvailable = true;
                    break;
                }
            }
        }

        auto promptRetry = []() -> bool {
            print("  Do you want to try again? (Y/N): ");
            string tryAgain;
            getline(cin, tryAgain);
            return (tryAgain == "Y" || tryAgain == "y");
        };

        if (!sizeAvailable) {
            println("\n  Error: Sorry, size '{}' is not available.", selected_size);
            if (promptRetry()) continue;
            else return false;
        }

        print("  Enter Start Date (DD/MM/YYYY): ");
        getline(cin, start);

        if (!tool::date::isValidFormat(start)) {
            println("\n  Error: Invalid date '{}'. Invalid date format or date does not exist.", start);
            if (promptRetry()) continue;
            else return false;
        }

        if (tool::date::isBeforeToday(start)) {
            println("\n  Error: Invalid date '{}'. Start date cannot be in the past.", start);
            if (promptRetry()) continue;
            else return false;
        }

        print("  Enter Expected Return Date (DD/MM/YYYY): ");
        getline(cin, end);

        if (!tool::date::isValidFormat(end)) {
            println("\n  Error: Invalid date '{}'. Invalid return date format or date does not exist.", end);
            if (promptRetry()) continue;
            else return false;
        }

        int total_days = tool::date::getDaysDifference(start, end);
        
        if (total_days < 0) {
            println("\n  Error: Return date cannot be before the start date.");
            if (promptRetry()) continue;
            else return false;
        }

        // Charge at least 1 day for same-day returns
        if (total_days == 0) total_days = 1;

        println("");
        println("  Total Days : {}", total_days);
        println("  Total Cost : RM {:.2f}", item.daily_rate * total_days);
        println("  Deposit    : RM 0.00");
        println("");

        print("  Confirm Rental? (Y/N): ");
        getline(cin, confirm);

        if (confirm == "Y" || confirm == "y") {
            auto txResult = transaction::rental::createRental(
                session.userid, item.catalog_id, selected_size, start, end, item.daily_rate, total_days
            );
            if (txResult) {
                println("\n  Success: Item successfully rented! Reference Number: {}", txResult.value());
            } else {
                println("\n  Error: {}", txResult.error());
            }
        } else {
            println("\n  Rental cancelled.");
        }

        println("\nPress Enter to return to catalog...");
        cin.get();
        return true; // Exit to catalog
    }
    return false;
}

void showItemDetails(const ::identity::auth::UserSession& session, const string &itemIdStr) {
  bool isCustomer = (std::find(session.roles.begin(), session.roles.end(), "customer") != session.roles.end());
  int catalog_id = 0;
  try {
      catalog_id = stoi(itemIdStr);
  } catch (...) {
      println("  Invalid Item ID format.");
      this_thread::sleep_for(chrono::milliseconds(1000));
      return;
  }

  auto itemOpt = inventory::apparel::getApparelById(catalog_id);
  if (!itemOpt) {
      println("  {}", itemOpt.error());
      this_thread::sleep_for(chrono::milliseconds(1000));
      return;
  }

  auto item = itemOpt.value();

  bool viewingItem = true;
  while (viewingItem) {
    tool::helper::clearScreen();
    tool::ui::displayTitle("ITEM DETAILS", 65);

    println("");
    println("  Catalog ID  : {}", item.catalog_id);
    println("  Name        : {}", item.name);
    println("  Category    : {}", item.category);
    println("  Rate        : RM {:.2f} / day", item.daily_rate);
    println("  Description : {}", item.description.empty() ? "None" : item.description);
    println("");
    println("  Colour      : {}", item.colour);
    println("");
    println("  Available Sizes in Stock:");
    auto sizesOpt = inventory::apparel::getAvailableSizes(catalog_id);
    if (sizesOpt && !sizesOpt.value().empty()) {
        for (const auto& [size, qty] : sizesOpt.value()) {
            println("  - {}  (Qty: {})", size, qty);
        }
    } else {
        println("  - Out of stock");
    }
    println("");
    tool::helper::drawLine(65, '-');

    if (isCustomer) {
        println("  [1] Rent this Item");
    }
    println("  [0] Go Back to Catalog");
    println("");
    print("  Enter your choice: ");

    string choice;
    if (!(cin >> choice)) {
      cin.clear();
      cin.ignore(1000, '\n');
      continue;
    }
    cin.ignore(1000, '\n');

    if (choice == "1" && isCustomer) {
      bool exitToCatalog = processRentalCheckout(session, item, catalog_id);
      if (exitToCatalog) {
          viewingItem = false;
      }
    } else if (choice == "0") {
      viewingItem = false;
    } else {
      println("  Invalid option.");
      this_thread::sleep_for(chrono::milliseconds(1000));
    }
  }
}

void showCatalog(const ::identity::auth::UserSession& session) {
  bool inCatalog = true;
  int currentPage = 1;
  const int itemsPerPage = 25;
  string searchTerm = "";

  while (inCatalog) {
    auto countRes = inventory::apparel::getTotalApparelsCount(searchTerm);
    int totalItems = countRes ? countRes.value() : 0;
    int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;
    
    if (currentPage > totalPages) currentPage = totalPages;
    if (currentPage < 1) currentPage = 1;

    tool::helper::clearScreen();
    vector<int> colWidths = {4, 25, 12, 10, 10};

    tool::ui::displayTitle("APPAREL CATALOG", 75);
    if (!searchTerm.empty()) {
        println("  [Search Filter: '{}']", searchTerm);
    }
    // Header Row
    tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "CATEGORY", "PRICE/DAY", "AVAILABLE"});
    tool::helper::drawLine(75, '-');

    int offset = (currentPage - 1) * itemsPerPage;
    auto result = inventory::apparel::getCatalogDisplay(itemsPerPage, offset, searchTerm);
    
    if (result) {
        for (const auto& item : result.value()) {
            tool::ui::printRow(colWidths, {
                to_string(item.catalog_id),
                item.name,
                item.category,
                format("RM {:.2f}", item.daily_rate),
                to_string(item.available_stock)
            });
        }
    } else {
        println("  Error fetching catalog: {}", result.error());
    }

    tool::helper::drawLine(75, '=');

    // Pagination
    if (totalPages > 1) {
      string pageInfo = format("Page {} of {} | Total: {}", currentPage, totalPages, totalItems);
      int padding = (75 - pageInfo.length()) / 2;
      string spaces(padding > 0 ? padding : 0, ' ');
      println("{}{}", spaces, pageInfo);

      tool::helper::drawLine(75, '-');
      println("  [N] Next Page     [P] Previous Page    [S] Search Catalog");
    } else {
      println("  [S] Search Catalog");
    }
    println("  [0] Go Back");
    println("");
    print("  Enter an Item ID to view details, or choose an option: ");

    string choice;
    if (!(cin >> choice)) {
      cin.clear();
      cin.ignore(1000, '\n');
      continue;
    }
    cin.ignore(1000, '\n');

    if (choice == "0") {
      inCatalog = false;
    } else if (choice == "N" || choice == "n") {
      if (currentPage < totalPages)
        currentPage++;
    } else if (choice == "P" || choice == "p") {
      if (currentPage > 1)
        currentPage--;
    } else if (choice == "S" || choice == "s") {
      print("  Enter search query (or press Enter to clear): ");
      getline(cin, searchTerm);
      currentPage = 1;
    } else {
      showItemDetails(session, choice);
    }
  }
}
} // namespace inventory::ui