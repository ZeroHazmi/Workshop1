#include "inventory/InventoryUI.h"
#include "identity/Auth/Auth.h"
#include "identity/Profile/Profile.h"
#include "transaction/Rental/Rental.h"
#include "inventory/Apparel/Apparel.h"
#include "tool/CLIComponents.h"
#include "tool/DateHelper.h"
#include "tool/helper.h"
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

    bool processRentalCheckout(const ::identity::auth::UserSession& session, const inventory::apparel::ApparelCatalog& item, int catalog_id) {
        bool renting = true;
        while (renting) {
            tool::helper::clearScreen();
            tool::ui::displayTitle("RENT ITEM", 65);
            println("  Item: {} (ID: {})", item.name, item.unique_id);
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
            start = tool::date::normalizeDateStr(start);

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
            end = tool::date::normalizeDateStr(end);

            int total_days = tool::date::getDaysDifference(start, end);
            
            if (total_days < 0) {
                println("\n  Error: Return date cannot be before the start date.");
                if (promptRetry()) continue;
                else return false;
            }

            // Charge at least 1 day for same-day returns
            if (total_days == 0) total_days = 1;

            double base_fee = item.daily_rate * total_days;
            // Calculate security deposit up front
            double deposit = max(50.00, base_fee * 0.50);
            double total_due = base_fee + deposit;

            println("");
            println("  Total Days : {}", total_days);
            println("  Total Cost : RM {:.2f}", base_fee);
            println("  Deposit    : RM {:.2f} (Held & refundable)", deposit);
            println("  Total Due  : RM {:.2f}", total_due);
            println("");

            print("  Confirm Rental? (Y/N): ");
            getline(cin, confirm);

            if (confirm == "Y" || confirm == "y") {
                auto txResult = transaction::rental::createRental(
                    session.userid, item.catalog_id, selected_size, start, end, item.daily_rate, total_days
                );
                if (txResult) {
                    std::print("\n  Success: Item successfully rented! Reference Number: {}\n", txResult.value());
                } else {
                    std::print("\n  Error: {}\n", txResult.error());
                }
            } else {
                println("\n  Rental cancelled.");
            }

            string waitInput;
            do {
                print("\nEnter '0' to return to catalog: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return true; // Exit to catalog
        }
        return false;
    }

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
        tool::helper::clearScreen();
        tool::ui::displayTitle("ITEM DETAILS", 65);

        println("");
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
        if (!(cin >> choice)) {
          cin.clear();
          cin.ignore(1000, '\n');
          invalidAttempts++;
          if (invalidAttempts >= 3) {
              println("\nToo many invalid attempts. Pausing for 5 seconds...");
              this_thread::sleep_for(chrono::seconds(5));
              invalidAttempts = 0;
          }
          continue;
        }
        cin.ignore(1000, '\n');

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
          invalidAttempts++;
          if (invalidAttempts >= 3) {
              println("\nToo many invalid attempts. Pausing for 5 seconds...");
              this_thread::sleep_for(chrono::seconds(5));
              invalidAttempts = 0;
          } else {
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

        tool::helper::clearScreen();
        tool::helper::drawLine(75, '=');
        tool::ui::displayTitle("APPAREL CATALOG", 75);
        tool::helper::drawLine(75, '=');
        if (!searchTerm.empty()) {
            println("  [Search Filter: '{}']", searchTerm);
        }
        println("");

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
        if (totalPages > 1) {
          string pageInfo = format("Page {} of {} | Total: {}", currentPage, totalPages, totalItems);
          int padding = (75 - static_cast<int>(pageInfo.length())) / 2;
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
          invalidAttempts++;
          if (invalidAttempts >= 3) {
              println("\nToo many invalid attempts. Pausing for 5 seconds...");
              this_thread::sleep_for(chrono::seconds(5));
              invalidAttempts = 0;
          }
          continue;
        }
        cin.ignore(1000, '\n');

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
              getline(cin, searchTerm);
              currentPage = 1;
            } else {
              showItemDetails(session, choice);
            }
        } else {
            println("  Invalid option or Item ID not found.");
            invalidAttempts++;
            if (invalidAttempts >= 3) {
                println("\nToo many invalid attempts. Pausing for 5 seconds...");
                this_thread::sleep_for(chrono::seconds(5));
                invalidAttempts = 0;
            } else {
                this_thread::sleep_for(chrono::milliseconds(1000));
            }
        }
      }
    }

    void registerNewApparel(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("REGISTER NEW APPAREL", 64);
        tool::helper::drawLine(64, '=');
        println("");

        auto staffProfileOpt = ::identity::profile::Profile::getStaffProfile(session.userid);
        if (!staffProfileOpt) {
            println("  Error: Could not retrieve staff profile.");
            string waitInput;
            do {
                print("\nEnter '0' to return to dashboard: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }
        int shop_id = staffProfileOpt.value().shop_id;

        string apparelName, category, colour, description;
        double rentalPrice;

        print("  Apparel Name (or '0' to cancel): ");
        getline(cin, apparelName);
        if (apparelName == "0" || apparelName == "cancel") return;
        
        print("  Description: ");
        getline(cin, description);
        
        print("  Category: ");
        getline(cin, category);
        
        print("  Colour: ");
        getline(cin, colour);
        
        print("  Rental Price (per day): RM ");
        if (!(cin >> rentalPrice) || rentalPrice < 0) {
            cin.clear(); cin.ignore(1000, '\n'); return;
        }
        cin.ignore(1000, '\n');

        std::vector<inventory::apparel::ItemBatch> batches;
        bool addingSizes = true;
        
        while (addingSizes) {
            println("\n  --- ADD INVENTORY BATCH ---");
            string size, condition, addAnother;
            int quantity;
            
            print("  Size (e.g. S, M, L): ");
            getline(cin, size);
            
            print("  Quantity: ");
            if (!(cin >> quantity) || quantity < 0) {
                cin.clear(); cin.ignore(1000, '\n'); 
                println("  Invalid quantity. Batch cancelled.");
                continue;
            }
            cin.ignore(1000, '\n');
            
            print("  Condition (e.g. Excellent, Good): ");
            getline(cin, condition);
            
            batches.push_back({size, quantity, condition});
            
            print("  Do you want to add another size for this apparel? (Y/N): ");
            getline(cin, addAnother);
            
            if (addAnother != "Y" && addAnother != "y") {
                addingSizes = false;
            }
        }

        if (batches.empty()) {
            println("\n  Registration cancelled: No inventory batches added.");
            string waitInput;
            do {
                print("\nEnter '0' to return to dashboard: ");
                getline(cin, waitInput);
            } while (waitInput != "0");
            return;
        }

        inventory::apparel::ApparelCatalog newCatalog;
        newCatalog.shop_id = shop_id;
        newCatalog.name = apparelName;
        newCatalog.description = description;
        newCatalog.category = category;
        newCatalog.colour = colour;
        newCatalog.daily_rate = rentalPrice;

        auto result = inventory::apparel::addApparelCatalog(newCatalog, batches);
        if (result) {
            int totalQty = 0;
            for (const auto& b : batches) totalQty += b.quantity;
            println("\n  Apparel registered successfully with {} total items!", totalQty);
        } else {
            println("\n  Error: {}", result.error());
        }
        
        string waitInput;
        do {
            print("\nEnter '0' to return to dashboard: ");
            getline(cin, waitInput);
        } while (waitInput != "0");
    }

} // namespace inventory::ui