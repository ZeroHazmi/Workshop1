#include "identity/Auth/Auth.h"
#include "identity/AdminUI.h"
#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "inventory/InventoryUI.h"
#include "identity/Profile/Profile.h"
#include <print>
#include <string>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::authui {
    // Forward declarations
    void viewProfile(const ::identity::auth::UserSession& session);
    void manageBankAccount(const ::identity::auth::UserSession& session);

    void showSplashScreen() {
        tool::helper::clearScreen();
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("FORMAL WEAR AND COSTUME RENTAL SYSTEM", 64);
        tool::ui::displayTitle("(FWCRS)", 64);
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("Premium attire management for students and university staff", 64);
        tool::helper::drawLine(64, '-');
        tool::ui::displayTitle("Location: Universiti Teknikal Malaysia Melaka (UTeM) ", 64);
        tool::helper::drawLine(64, '=');

        // for (int i = 0; i < 3; ++i) {
        //     this_thread::sleep_for(chrono::milliseconds(700));
        //     print(".");
        // }
        println("\n");

        // tool::helper::clearScreen();
    }

    void showCustomerDashboard(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        
        // Header with double-line borders
        tool::helper::drawLine(64, '=');
        tool::ui::displayTitle("CUSTOMER DASHBOARD", 64);
        tool::helper::drawLine(64, '=');

        // User Context
        println("\n  Welcome back, {}!", session.username);
        println("  Role: {}\n", session.roles.front()); // Displays the primary role
        
        // Vertical Menu Options
        println("  [1] Browse Apparel Inventory");
        println("  [2] View My Rental History");
        println("  [3] Update Personal Profile");
        println("  [4] Manage Bank Account Details");
        println("  [0] Logout");
        
        // Footer decoration
        tool::helper::drawLine(64, '-');
        print("  Select an option: ");
    }

    void handleCustomerDashboard(const ::identity::auth::UserSession& session) {
        bool inDashboard = true;
        while (inDashboard) {
            showCustomerDashboard(session);
            
            int subChoice;
            if (!(cin >> subChoice)) {
                cin.clear();
                cin.ignore(1000, '\n');
                continue;
            }
            cin.ignore(1000, '\n');  // Clear the input buffer after reading choice

            switch (subChoice) {
                case 1: 
                    // inventory::browseApparel
                    inventory::ui::showCatalog(session);
                    break;
                case 2: 
                    // transaction::viewHistory(session.userid); 
                    break;
                case 3: 
                    // profile::updateProfile(session.userid);
                    viewProfile(session); 
                    break;
                case 4: 
                    // profile::manageBank(session.userid);
                    manageBankAccount(session);
                    break;
                case 0:
                    println("\nLogging out...");
                    inDashboard = false;

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

    void viewProfile(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("USER PROFILE", 50);
        println(""); // Spacer

        auto profileOpt = ::identity::profile::Profile::getCustomerProfile(session.userid);
        if (profileOpt) {
            auto& profile = profileOpt.value();
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Full Name", profile.fullname);
            tool::ui::printField("Email", profile.email);
            tool::ui::printField("Phone", profile.phone_no);
        } else {
            println("  Error: {}", profileOpt.error());
            println("  (Please ensure your customer profile has been fully set up.)");
        }
        
        println("");
        tool::helper::drawLine(50, '-');
        println("\nPress Enter to return to dashboard...");
        cin.get();
    }

    void manageBankAccount(const ::identity::auth::UserSession& session) {
        tool::helper::clearScreen();
        tool::ui::displayTitle("BANK ACCOUNT DETAILS", 50);
        println(""); // Spacer

        auto bankOpt = ::identity::profile::Profile::getBankAccount(session.userid);
        if (bankOpt) {
            auto& bank = bankOpt.value();
            tool::ui::printField("Account Holder", bank.acc_holder);
            tool::ui::printField("Bank Name", bank.bank_name);
            
            // Mask account number for security: e.g. show only last 4 digits
            std::string acc = bank.acc_number;
            if (acc.length() > 4) {
                acc = std::string(acc.length() - 4, '*') + acc.substr(acc.length() - 4);
            }
            tool::ui::printField("Account Number", acc);
            
            println("");
            tool::helper::drawLine(50, '-');
            println("\n  [1] Remove Linked Bank Account");
            println("  [0] Back to Dashboard");
            print("\n  Enter selection: ");
            int option;
            if (cin >> option && option == 1) {
                cin.ignore(1000, '\n');
                auto removeRes = ::identity::profile::Profile::removeBankAccount(bank.acc_id);
                if (removeRes) {
                    println("\n  Bank account successfully unlinked.");
                } else {
                    println("\n  Failed to unlink bank account: {}", removeRes.error());
                }
                println("\nPress Enter to return to dashboard...");
                cin.get();
                return;
            } else {
                cin.clear();
                cin.ignore(1000, '\n');
                return;
            }
        } else {
            println("  No bank account linked to your profile yet.");
            println("\n  [1] Link a Bank Account");
            println("  [0] Back to Dashboard");
            print("\n  Enter selection: ");
            
            int choice;
            if (cin >> choice && choice == 1) {
                cin.ignore(1000, '\n');
                tool::helper::clearScreen();
                tool::ui::displayTitle("LINK BANK ACCOUNT", 50);
                println("");
                
                std::string bankName, accNum, holder;
                print("  Enter Bank Name: ");
                getline(cin, bankName);
                print("  Enter Account Number: ");
                getline(cin, accNum);
                print("  Enter Account Holder Name: ");
                getline(cin, holder);
                
                auto linkResult = ::identity::profile::Profile::linkBankAccount(session.userid, bankName, accNum, holder);
                if (linkResult) {
                    println("\n  Bank account linked successfully!");
                } else {
                    println("\n  Failed to link bank account: {}", linkResult.error());
                }
                println("\nPress Enter to return to dashboard...");
                cin.get();
                return;
            } else {
                cin.clear();
                cin.ignore(1000, '\n');
                return;
            }
        }
    }

    void showAdminDashboard(const ::identity::auth::UserSession& session) {
        // Delegate to admin UI
        ::identity::adminui::showAdminDashboard(session);
    }

    void handleAdminDashboard(const ::identity::auth::UserSession& session) {
        // Delegate to admin UI
        ::identity::adminui::handleAdminDashboard(session);
    }

}