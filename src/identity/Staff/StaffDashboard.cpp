#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include "identity/Profile/Profile.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <print>
#include <string>
#include <iostream>
#include <thread>

namespace auth = ::identity::auth;
namespace profile = ::identity::profile;
namespace db = ::database;

using namespace std;

namespace identity::staffui {

    void showStaffDashboard(const auth::UserSession& session) {
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
        println("  [2] Manage Active Rentals & Returns");
        println("  [3] View Staff Profile");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleStaffDashboard(const auth::UserSession& session) {
        bool inStaffPanel = true;
        while (inStaffPanel) {
            showStaffDashboard(session);
            
            int choice;
            if (!tool::input::readInt(choice)) {
                println("  Invalid selection.");
                this_thread::sleep_for(chrono::milliseconds(1000));
                continue;
            }

            switch (choice) {
                case 1:
                    manageApparelInventory(session);
                    break;
                case 2:
                    manageRentalsAndReturns(session);
                    break;
                case 3:
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
                    println("  Invalid option.");
                    this_thread::sleep_for(chrono::milliseconds(1000));
                    break;
            }
        }
    }

    void viewStaffProfile(const auth::UserSession& session) {
        tool::ui::showHeader("STAFF PROFILE", 64);

        auto profileRes = profile::Profile::getStaffProfile(session.userid);
        if (profileRes) {
            auto staff = profileRes.value();
            tool::ui::printField("Staff Name", staff.staff_name);
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Role", session.roles.front());
            tool::ui::printField("Position", staff.position);
            tool::ui::printField("Phone Number", staff.phone_no);
            
            // Query shop name dynamically using shop_id
            string shopName = "Not Assigned";
            if (staff.shop_id > 0) {
                auto& db = db::DatabaseManager::getInstance();
                string shopQuery = "SELECT shop_name FROM shops WHERE shop_id = " + to_string(staff.shop_id) + ";";
                auto shopRes = db.executeQuery(shopQuery);
                if (shopRes) {
                    sql::ResultSet* srs = shopRes.value();
                    if (srs->next()) {
                        shopName = srs->getString("shop_name");
                    }
                    delete srs;
                }
            }
            tool::ui::printField("Assigned Shop", shopName);
        } else {
            // Graceful fallback to formatted system values if profile is absent
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Role", session.roles.front());
            tool::ui::printField("Staff Email", session.username + "@utem.edu.my");
            tool::ui::printField("Department", "Rental Management");
            tool::ui::printField("Assigned Shop", "Not Assigned");
        }
        
        println("\n  (Note: Contact admin to modify profile information)");
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

}