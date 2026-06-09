#include "identity/AuthUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include "identity/Profile/Profile.h"
#include <print>
#include <string>
#include <format>
#include <iostream>
#include <thread>

using namespace std;

namespace identity::authui {

    void manageBankAccount(const ::identity::auth::UserSession& session) {
        bool inBankMenu = true;
        int invalidAttempts = 0;
        while (inBankMenu) {
            tool::ui::showHeader("BANK ACCOUNT DETAILS", 64);
            
            auto bankOpt = ::identity::profile::Profile::getBankAccount(session.userid);
            if (bankOpt) {
                auto& bank = bankOpt.value();
                tool::ui::printField("Account ID", bank.unique_id);
                tool::ui::printField("Account Holder", bank.acc_holder);
                tool::ui::printField("Bank Name", bank.bank_name);
                
                // Mask account number for security: e.g. show only last 4 digits
                std::string acc = bank.acc_number;
                if (acc.length() > 4) {
                    acc = std::string(acc.length() - 4, '*') + acc.substr(acc.length() - 4);
                }
                tool::ui::printField("Account Number", acc);
                tool::ui::printField("Balance", std::format("RM {:.2f}", bank.balance));
                
                println("");
                tool::helper::drawLine(50, '-');
                println("\n  [1] Deposit Funds / Add Balance");
                println("  [2] Remove Linked Bank Account");
                println("  [0] Back to Dashboard");
                print("\n  Enter selection: ");
                int option;
                if (tool::input::readInt(option)) {
                    if (option == 1) {
                        invalidAttempts = 0;
                        print("  Enter deposit amount: RM ");
                        double amount;
                        if (cin >> amount && amount > 0) {
                            cin.ignore(1000, '\n');
                            auto depositRes = ::identity::profile::Profile::depositBalance(bank.acc_id, amount);
                            if (depositRes) {
                                println("\n  Successfully deposited RM {:.2f} to your account!", amount);
                            } else {
                                println("\n  Deposit failed: {}", depositRes.error());
                            }
                        } else {
                            cin.clear();
                            cin.ignore(1000, '\n');
                            println("\n  Invalid deposit amount.");
                        }
                        tool::ui::pressZeroToReturn("bank menu", 64);
                    } else if (option == 2) {
                        invalidAttempts = 0;
                        auto removeRes = ::identity::profile::Profile::removeBankAccount(bank.acc_id);
                        if (removeRes) {
                            println("\n  Bank account successfully unlinked.");
                        } else {
                            println("\n  Failed to unlink bank account: {}", removeRes.error());
                        }
                        tool::ui::pressZeroToReturn("bank menu", 64);
                    } else if (option == 0) {
                        invalidAttempts = 0;
                        inBankMenu = false;
                    } else {
                        println("  Invalid selection.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                    }
                } else {
                    println("  Invalid selection.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
                }
            } else {
                println("  No bank account linked to your profile yet.");
                println("\n  [1] Link a Bank Account");
                println("  [0] Back to Dashboard");
                print("\n  Enter selection: ");
                
                int choice;
                if (tool::input::readInt(choice)) {
                    if (choice == 1) {
                        invalidAttempts = 0;
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
                        tool::ui::pressZeroToReturn("bank menu", 64);
                    } else if (choice == 0) {
                        invalidAttempts = 0;
                        inBankMenu = false;
                    } else {
                        println("  Invalid selection.");
                        if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                            this_thread::sleep_for(chrono::milliseconds(1000));
                        }
                    }
                } else {
                    println("  Invalid selection.");
                    if (!tool::ui::handleInvalidAttempt(invalidAttempts)) {
                        this_thread::sleep_for(chrono::milliseconds(1000));
                    }
                }
            }
        }
    }

}
