#include "inventory/InventoryUI.h"
#include "identity/Auth/Auth.h"
#include "identity/Profile/Profile.h"
#include "inventory/Apparel/Apparel.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "tool/input.h"
#include <algorithm>
#include <format>
#include <iostream>
#include <print>
#include <string>
#include <vector>

namespace auth = ::identity::auth;
namespace profile = ::identity::profile;
namespace apparel = ::inventory::apparel;

using namespace std;

namespace inventory::ui {

    void registerNewApparel(const auth::UserSession& session) {
        tool::ui::showHeader("REGISTER NEW APPAREL", 64);

        auto staffProfileOpt = profile::Profile::getStaffProfile(session.userid);
        if (!staffProfileOpt) {
            println("  Error: Could not retrieve staff profile.");
            tool::ui::pressZeroToReturn("dashboard", 64);
            return;
        }
        int shop_id = staffProfileOpt.value().shop_id;

        string apparelName, category, colour, description;
        double rentalPrice = 0.0;

        print("  Apparel Name (or '0' to cancel): ");
        if (!tool::input::readLine(apparelName)) return;
        if (apparelName == "0" || apparelName == "cancel") return;
        
        print("  Description: ");
        if (!tool::input::readLine(description)) return;
        
        print("  Category: ");
        if (!tool::input::readLine(category)) return;
        
        print("  Colour: ");
        if (!tool::input::readLine(colour)) return;
        
        print("  Rental Price (per day): RM ");
        string priceStr;
        if (!tool::input::readLine(priceStr)) return;
        try {
            rentalPrice = stod(priceStr);
            if (rentalPrice < 0) {
                println("  Invalid price. Registration cancelled.");
                return;
            }
        } catch (...) {
            println("  Invalid price. Registration cancelled.");
            return;
        }

        vector<apparel::ItemBatch> batches;
        bool addingSizes = true;
        
        while (addingSizes) {
            println("\n  --- ADD INVENTORY BATCH ---");
            string size, condition, addAnother;
            int quantity = 0;
            
            print("  Size (e.g. S, M, L): ");
            if (!tool::input::readLine(size)) continue;
            
            print("  Quantity: ");
            string qtyStr;
            if (!tool::input::readLine(qtyStr)) {
                println("  Invalid quantity. Batch cancelled.");
                continue;
            }
            try {
                quantity = stoi(qtyStr);
                if (quantity < 0) {
                    println("  Invalid quantity. Batch cancelled.");
                    continue;
                }
            } catch (...) {
                println("  Invalid quantity. Batch cancelled.");
                continue;
            }
            
            print("  Condition (e.g. Excellent, Good): ");
            if (!tool::input::readLine(condition)) continue;
            
            batches.push_back({size, quantity, condition});
            
            print("  Do you want to add another size for this apparel? (Y/N): ");
            if (!tool::input::readLine(addAnother)) {
                addingSizes = false;
            }
            
            if (addAnother != "Y" && addAnother != "y") {
                addingSizes = false;
            }
        }

        if (batches.empty()) {
            println("\n  Registration cancelled: No inventory batches added.");
            tool::ui::pressZeroToReturn("dashboard", 64);
            return;
        }

        apparel::ApparelCatalog newCatalog;
        newCatalog.shop_id = shop_id;
        newCatalog.name = apparelName;
        newCatalog.description = description;
        newCatalog.category = category;
        newCatalog.colour = colour;
        newCatalog.daily_rate = rentalPrice;

        auto result = apparel::addApparelCatalog(newCatalog, batches);
        if (result) {
            int totalQty = 0;
            for (const auto& b : batches) totalQty += b.quantity;
            println("\n  Apparel registered successfully with {} total items!", totalQty);
        } else {
            println("\n  Error: {}", result.error());
        }
        
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

}