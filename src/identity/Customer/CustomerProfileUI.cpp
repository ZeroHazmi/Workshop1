#include "identity/AuthUI.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "tool/input.h"
#include "identity/Profile/Profile.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>
#include <cppconn/resultset.h>

namespace auth = ::identity::auth;
namespace profile = ::identity::profile;
namespace db = ::database;

using namespace std;

namespace identity::authui {

    void viewProfile(const auth::UserSession& session) {
        bool inProfileMenu = true;
        while (inProfileMenu) {
            tool::ui::showHeader("USER PROFILE", 64);

            auto profileOpt = profile::Profile::getCustomerProfile(session.userid);
            if (profileOpt) {
                auto profile = profileOpt.value();
                tool::ui::printField("Username", session.username);
                tool::ui::printField("Full Name", profile.fullname);
                tool::ui::printField("Email", profile.email);
                tool::ui::printField("Phone", profile.phone_no);

                println("");
                tool::helper::drawLine(64, '-');
                println("  [1] Update Full Name");
                println("  [2] Update Email Address");
                println("  [3] Update Phone Number");
                println("  [0] Return to Dashboard");
                tool::helper::drawLine(64, '-');
                print("  Select an option: ");

                int option;
                if (tool::input::readInt(option)) {
                    switch (option) {
                        case 1: {
                            print("\n  Enter New Full Name: ");
                            string newName;
                            getline(cin, newName);
                            if (!newName.empty()) {
                                auto updateRes = profile::Profile::updateCustomerProfile(session.userid, newName, profile.email, profile.phone_no);
                                if (updateRes) {
                                    println("\n  Full Name updated successfully!");
                                } else {
                                    println("\n  [Error] Failed to update Full Name: {}", updateRes.error());
                                }
                            } else {
                                println("\n  Update cancelled (empty input).");
                            }
                            this_thread::sleep_for(chrono::milliseconds(1500));
                            break;
                        }
                        case 2: {
                            print("\n  Enter New Email: ");
                            string newEmail;
                            getline(cin, newEmail);
                            if (!newEmail.empty()) {
                                // Validate duplicate email in the CUSTOMERS table (excluding the current user's email)
                                auto& dbInst = db::DatabaseManager::getInstance();
                                string checkQuery = "SELECT COUNT(*) FROM CUSTOMERS WHERE email = '" + newEmail + "' AND user_id != " + to_string(session.userid) + " AND is_deleted = 0;";
                                auto checkRes = dbInst.executeQuery(checkQuery);
                                bool isDuplicate = false;
                                if (checkRes) {
                                    sql::ResultSet* rs = checkRes.value();
                                    if (rs->next() && rs->getInt(1) > 0) {
                                        isDuplicate = true;
                                    }
                                    delete rs;
                                }
                                if (isDuplicate) {
                                    println("\n  [Error] Email address is already registered by another customer.");
                                } else {
                                    auto updateRes = profile::Profile::updateCustomerProfile(session.userid, profile.fullname, newEmail, profile.phone_no);
                                    if (updateRes) {
                                        println("\n  Email updated successfully!");
                                    } else {
                                        println("\n  [Error] Failed to update Email: {}", updateRes.error());
                                    }
                                }
                            } else {
                                println("\n  Update cancelled (empty input).");
                            }
                            this_thread::sleep_for(chrono::milliseconds(1500));
                            break;
                        }
                        case 3: {
                            print("\n  Enter New Phone Number: ");
                            string newPhone;
                            getline(cin, newPhone);
                            if (!newPhone.empty()) {
                                auto updateRes = profile::Profile::updateCustomerProfile(session.userid, profile.fullname, profile.email, newPhone);
                                if (updateRes) {
                                    println("\n  Phone number updated successfully!");
                                } else {
                                    println("\n  [Error] Failed to update Phone: {}", updateRes.error());
                                }
                            } else {
                                println("\n  Update cancelled (empty input).");
                            }
                            this_thread::sleep_for(chrono::milliseconds(1500));
                            break;
                        }
                        case 0:
                            return;
                        default:
                            println("  Invalid selection.");
                            this_thread::sleep_for(chrono::milliseconds(1000));
                    }
                } else {
                    println("  Invalid selection.");
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            } else {
                println("  Error: {}", profileOpt.error());
                println("  (Please ensure your customer profile has been fully set up.)");
                tool::ui::pressZeroToReturn("dashboard", 64);
                inProfileMenu = false;
            }
        }
    }

}