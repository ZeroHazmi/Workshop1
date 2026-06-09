#include "identity/Auth/Auth.h"
#include "identity/Customer/Customer.h"
#include "identity/Staff/Staff.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>
#include <thread>
#include <chrono>
#include <iostream>
#include <conio.h>
#include "tool/helper.h"

namespace db = ::database;

using namespace std;

namespace identity::auth {

    string Auth::getMaskedPassword() {
        string password;
        char ch;
        
        while ((ch = _getch()) != '\r') { // '\r' is the Enter key in conio
            if (ch == '\b') { // Handle Backspace
                if (!password.empty()) {
                    password.pop_back();
                    print("\b \b"); // Move cursor back, overwrite with space, move back again
                }
            } else if (ch >= 32 && ch <= 126) { // Printable characters only
                password += ch;
                print("*");
            }
        }
        println(""); // Move to next line after Enter
        return password;
    }

    // This handles the user interaction for Login
    expected<UserSession, string> Auth::handleLoginFlow() {
        string user, pass;
        
        println("\n--- USER LOGIN ---");
        print("Username: ");
        cin >> user;
        print("Password: ");
        pass = getMaskedPassword();

        auto result = loginUser(user, pass);
        if (!result) {
            println("\nError: {}", result.error());
            this_thread::sleep_for(chrono::milliseconds(1500));
        }
        return result;
    }

    // This handles the user interaction for Registration
    void Auth::handleRegisterFlow(string_view role) {
        string user, pass;
        
        println("\n--- NEW USER REGISTRATION ---");
        print("Username (or '0' to cancel): "); 
        cin >> user;
        if (user == "0") {
            println("\nRegistration cancelled.");
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
            return;
        }
        print("Password: ");
        pass = getMaskedPassword();
        cin.ignore(numeric_limits<streamsize>::max(), '\n');

        // Register user account with specified role
        auto userResult = registerUser(user, pass, role);
        if (!userResult) {
            println("\nRegistration failed: {}", userResult.error());
            return;
        }

        int userId = userResult.value();

        // Query database to get the user's actual role
        string roleQuery = "SELECT roles FROM USERS WHERE user_id = " + to_string(userId) + ";";
        auto roleResult = db::DatabaseManager::getInstance().executeQuery(roleQuery);
        
        if (!roleResult) {
            println("\nError retrieving user role: {}", roleResult.error());
            return;
        }

        sql::ResultSet* rs = roleResult.value();
        string userRole = "customer"; // Default role
        
        if (rs->next()) {
            userRole = rs->getString("roles");
        }
        delete rs;

        // Based on role, collect appropriate profile information and create profile
        if (userRole == "customer") {
            string fullname, email, phone;
            
            print("Full Name: ");
            getline(cin, fullname);
            
            print("Email: ");
            cin >> email;
            
            print("Phone: ");
            cin >> phone;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            // Create customer profile
            auto profileResult = ::identity::customer::createCustomerProfile(userId, fullname, email, phone);
            
            if (profileResult) {
                println("\nCustomer registration successful! You can now login.");
            } else {
                println("\nProfile creation failed: {}", profileResult.error());
            }
        } 
        else if (userRole == "staff") {
            string staffName, position, phone;
            int shopId;
            
            print("Full Name: ");
            getline(cin, staffName);
            
            print("Position: ");
            getline(cin, position);
            
            print("Phone: ");
            cin >> phone;
            
            print("Shop ID: ");
            cin >> shopId;
            cin.ignore(numeric_limits<streamsize>::max(), '\n');

            // Create staff profile
            auto profileResult = ::identity::staff::createStaffProfile(userId, shopId, staffName, position, phone);
            
            if (profileResult) {
                println("\nStaff registration successful! You can now login.");
            } else {
                println("\nProfile creation failed: {}", profileResult.error());
            }
        }
        else {
            println("\nUnknown role: {}. Please contact administrator.", userRole);
        }
    }

    bool Auth::validatePassword(string_view password) {
        // Basic password validation: at least 6 characters
        return password.length() >= 6;
    }

    expected<UserSession, string> Auth::loginUser(
        string_view username,
        string_view password
    ) {
        if (username.empty() || password.empty()) {
            return unexpected("Username and password cannot be empty.");
        }

        // Query: SELECT user_id, username, password, roles FROM USERS WHERE username = ? AND is_deleted = 0
        string query = "SELECT user_id, username, password, roles FROM USERS WHERE username = '" 
                        + string(username) + "' AND is_deleted = 0;";

        auto result = db::DatabaseManager::getInstance().executeQuery(query);

        if (!result) {
            return unexpected(result.error());
        }

        sql::ResultSet* rs = result.value();

        if (rs->next()) {
            string storedPassword = rs->getString("password");
            int userid = rs->getInt("user_id");
            string rolesStr = rs->getString("roles");

            // Simple password validation (in production, use hashing!)
            if (storedPassword == string(password)) {
                UserSession session;
                session.userid = userid;
                session.username = username;

                // Parse roles (comma-separated)
                if (!rolesStr.empty()) {
                    size_t pos = 0;
                    while ((pos = rolesStr.find(',')) != string::npos) {
                        session.roles.push_back(rolesStr.substr(0, pos));
                        rolesStr.erase(0, pos + 1);
                    }
                    session.roles.push_back(rolesStr);
                }

                delete rs;
                return session;
            } else {
                delete rs;
                return unexpected("Invalid password.");
            }
        } else {
            delete rs;
            return unexpected("User not found.");
        }
    }

    expected<int, string> Auth::registerUser(
        string_view username,
        string_view password,
        string_view role
    ) {
        if (username.empty() || password.empty()) {
            return unexpected("Username and password are required.");
        }

        if (!validatePassword(password)) {
            return unexpected("Password must be at least 6 characters.");
        }

        // Check if username already exists
        string checkQuery = "SELECT user_id FROM USERS WHERE username = '" 
                                + string(username) + "';";
        auto checkResult = db::DatabaseManager::getInstance().executeQuery(checkQuery);
        
        if (checkResult && checkResult.value()->next()) {
            delete checkResult.value();
            return unexpected("Username already exists.");
        }
        if (checkResult && checkResult.value()) {
            delete checkResult.value();
        }

        // Insert new user with specified role
        string insertQuery = 
            "INSERT INTO USERS (username, password, roles) VALUES ('" +
            string(username) + "', '" +
            string(password) + "', '" + string(role) +"');";

        auto insertResult = db::DatabaseManager::getInstance().executeUpdate(insertQuery);

        if (!insertResult) {
            return unexpected(insertResult.error());
        }

        // Query to get the new user_id
        string selectQuery = "SELECT user_id FROM USERS WHERE username = '" 
                                + string(username) + "';";
        auto selectResult = db::DatabaseManager::getInstance().executeQuery(selectQuery);

        if (!selectResult) {
            return unexpected(selectResult.error());
        }

        sql::ResultSet* rs = selectResult.value();
        if (rs->next()) {
            int newUserId = rs->getInt("user_id");
            delete rs;
            return newUserId;
        } else {
            delete rs;
            return unexpected("Failed to retrieve new user ID.");
        }
    }

}