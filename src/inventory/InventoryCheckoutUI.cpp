#include "inventory/InventoryInternal.h"
#include "identity/Auth/Auth.h"
#include "transaction/Rental/Rental.h"
#include "inventory/Apparel/Apparel.h"
#include "tool/CLIComponents.h"
#include "tool/DateHelper.h"
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

namespace auth = ::identity::auth;
namespace apparel = ::inventory::apparel;
namespace rental = ::transaction::rental;

using namespace std;

namespace inventory::ui {

    bool processRentalCheckout(const auth::UserSession& session, const apparel::ApparelCatalog& item, int catalog_id) {
        bool renting = true;
        while (renting) {
            tool::ui::showHeader("RENT ITEM", 65);
            println("  Item: {} (ID: {})", item.name, item.unique_id);
            println("");

            string selected_size, start, end, confirm;
            print("  Enter Size you wish to rent: ");
            tool::input::readLine(selected_size);
            
            bool sizeAvailable = false;
            auto currentSizesOpt = apparel::getAvailableSizes(catalog_id);
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
                tool::input::readLine(tryAgain);
                return (tryAgain == "Y" || tryAgain == "y");
            };

            if (!sizeAvailable) {
                println("\n  Error: Sorry, size '{}' is not available.", selected_size);
                if (promptRetry()) continue;
                else return false;
            }

            print("  Enter Start Date (DD/MM/YYYY): ");
            tool::input::readLine(start);

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
            tool::input::readLine(end);

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
            tool::input::readLine(confirm);

            if (confirm == "Y" || confirm == "y") {
                auto txResult = rental::createRental(
                    session.userid, item.catalog_id, selected_size, start, end, item.daily_rate, total_days
                );
                if (txResult) {
                    print("\n  Success: Item successfully rented! Reference Number: {}\n", txResult.value());
                } else {
                    print("\n  Error: {}\n", txResult.error());
                }
            } else {
                println("\n  Rental cancelled.");
            }

            tool::ui::pressZeroToReturn("catalog", 65);
            return true; // Exit to catalog
        }
        return false;
    }

}