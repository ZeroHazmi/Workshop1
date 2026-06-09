#include "inventory/InventoryUI.h"
#include "inventory/InventoryInternal.h"
#include "identity/Auth/Auth.h"
#include "inventory/Apparel/Apparel.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "tool/input.h"
#include <algorithm>
#include <chrono>
#include <format>
#include <iostream>
#include <print>
#include <string>
#include <thread>
#include <vector>

using namespace std;

namespace inventory::ui {

    void showItemDetails(const ::identity::auth::UserSession& session, const string &itemIdStr) {
      bool isCustomer = (find(session.roles.begin(), session.roles.end(), "customer") != session.roles.end());

      auto itemOpt = inventory::apparel::getApparelByUniqueId(itemIdStr);
      if (!itemOpt) {
          println("  {}", itemOpt.error());
          this_thread::sleep_for(chrono::milliseconds(1000));
          return;
      }

      auto item = itemOpt.value();
      int catalog_id = item.catalog_id;

      bool viewingItem = true;
      int invalidAttempts = 0;
      while (viewingItem) {
        tool::ui::showHeader("ITEM DETAILS", 65);

        println("  Catalog ID  : {}", item.unique_id);
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
        if (!tool::input::readLine(choice)) {
          tool::ui::handleInvalidAttempt(invalidAttempts);
          continue;
        }

        if (choice == "1" && isCustomer) {
          invalidAttempts = 0;
          bool exitToCatalog = processRentalCheckout(session, item, catalog_id);
          if (exitToCatalog) {
              viewingItem = false;
          }
        } else if (choice == "0") {
          invalidAttempts = 0;
          viewingItem = false;
        } else {
          println("  Invalid option.");
          if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
              this_thread::sleep_for(chrono::milliseconds(1000));
          }
        }
      }
    }

    void showCatalog(const ::identity::auth::UserSession& session) {
      bool inCatalog = true;
      int currentPage = 1;
      const int itemsPerPage = 25;
      string searchTerm = "";
      int invalidAttempts = 0;

      while (inCatalog) {
        auto countRes = inventory::apparel::getTotalApparelsCount(searchTerm);
        int totalItems = countRes ? countRes.value() : 0;
        int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;
        
        if (currentPage > totalPages) currentPage = totalPages;
        if (currentPage < 1) currentPage = 1;

        tool::ui::showHeader("APPAREL CATALOG", 75);
        if (!searchTerm.empty()) {
            println("  [Search Filter: '{}']", searchTerm);
        }

        vector<int> colWidths = {12, 24, 19, 10, 10};

        // Header Row
        tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "CATEGORY", "PRICE/DAY", "AVAILABLE"});
        tool::helper::drawLine(75, '-');

        int offset = (currentPage - 1) * itemsPerPage;
        auto result = inventory::apparel::getCatalogDisplay(itemsPerPage, offset, searchTerm);
        
        if (result) {
            for (const auto& item : result.value()) {
                tool::ui::printRow(colWidths, {
                    item.unique_id,
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
        tool::ui::printPaginationFooter(currentPage, totalPages, totalItems, 75);
        println("  [S] Search Catalog");
        println("  [0] Go Back");
        println("");
        print("  Enter an Item ID to view details, or choose an option: ");

        string choice;
        if (!tool::input::readLine(choice)) {
          tool::ui::handleInvalidAttempt(invalidAttempts);
          continue;
        }

        bool validChoice = false;
        if (choice == "0" || choice == "N" || choice == "n" || choice == "P" || choice == "p" || choice == "S" || choice == "s") {
            validChoice = true;
        } else {
            auto itemOpt = inventory::apparel::getApparelByUniqueId(choice);
            if (itemOpt) {
                validChoice = true;
            }
        }

        if (validChoice) {
            invalidAttempts = 0;
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
              tool::input::readLine(searchTerm);
              currentPage = 1;
            } else {
              showItemDetails(session, choice);
            }
        } else {
            println("  Invalid option or Item ID not found.");
            if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
      }
    }

}
