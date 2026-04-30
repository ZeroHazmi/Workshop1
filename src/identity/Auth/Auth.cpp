#include "identity/Auth/Auth.h"
#include "identity/Customer/Customer.h"
#include "identity/Staff/Staff.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>
#include <thread>
#include <chrono>
#include <iostream>
#include <conio.h>

namespace identity::auth {

    std::string Auth::getMaskedPassword() {
        std::string password;
        char ch;
        
        while ((ch = _getch()) != '\r') { // '\r' is the Enter key in conio
            if (ch == '\b') { // Handle Backspace
                if (!password.empty()) {
                    password.pop_back();
                    std::print("\b \b"); // Move cursor back, overwrite with space, move back again
                }
            } else if (ch >= 32 && ch <= 126) { // Printable characters only
                password += ch;
                std::print("*");
            }
        }
        std::println(""); // Move to next line after Enter
        return password;
    }

    // This handles the user interaction for Login
    std::expected<UserSession, std::string> Auth::handleLoginFlow() {
        std::string user, pass;
        
        std::println("\n--- USER LOGIN ---");
        std::print("Username: ");
        std::cin >> user;
        std::print("Password: ");
        pass = getMaskedPassword();

        auto result = loginUser(user, pass);
        if (!result) {
            std::println("\nError: {}", result.error());
        }
        return result;
    }

    // This handles the user interaction for Registration
    void Auth::handleRegisterFlow(std::string_view role) {
        std::string user, pass;
        
        std::println("\n--- NEW USER REGISTRATION ---");
        std::print("Username: "); 
        std::cin >> user;
        std::print("Password: ");
        pass = getMaskedPassword();
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

        // Register user account with specified role
        auto userResult = registerUser(user, pass, role);
        if (!userResult) {
            std::println("\nRegistration failed: {}", userResult.error());
            return;
        }

        int userId = userResult.value();

        // Query database to get the user's actual role
        std::string roleQuery = "SELECT roles FROM USERS WHERE user_id = " + std::to_string(userId) + ";";
        auto roleResult = database::DatabaseManager::getInstance().executeQuery(roleQuery);
        
        if (!roleResult) {
            std::println("\nError retrieving user role: {}", roleResult.error());
            return;
        }

        sql::ResultSet* rs = roleResult.value();
        std::string userRole = "customer"; // Default role
        
        if (rs->next()) {
            userRole = rs->getString("roles");
        }
        delete rs;

        // Based on role, collect appropriate profile information and create profile
        if (userRole == "customer") {
            std::string fullname, email, phone;
            
            std::print("Full Name: ");
            std::getline(std::cin, fullname);
            
            std::print("Email: ");
            std::cin >> email;
            
            std::print("Phone: ");
            std::cin >> phone;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // Create customer profile
            auto profileResult = identity::customer::createCustomerProfile(userId, fullname, email, phone);
            
            if (profileResult) {
                std::println("\nCustomer registration successful! You can now login.");
            } else {
                std::println("\nProfile creation failed: {}", profileResult.error());
            }
        } 
        else if (userRole == "staff") {
            std::string staffName, position, phone;
            int shopId;
            
            std::print("Full Name: ");
            std::getline(std::cin, staffName);
            
            std::print("Position: ");
            std::getline(std::cin, position);
            
            std::print("Phone: ");
            std::cin >> phone;
            
            std::print("Shop ID: ");
            std::cin >> shopId;
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

            // Create staff profile
            auto profileResult = identity::staff::createStaffProfile(userId, shopId, staffName, position, phone);
            
            if (profileResult) {
                std::println("\nStaff registration successful! You can now login.");
            } else {
                std::println("\nProfile creation failed: {}", profileResult.error());
            }
        }
        else {
            std::println("\nUnknown role: {}. Please contact administrator.", userRole);
        }
    }

    bool Auth::validatePassword(std::string_view password) {
        // Basic password validation: at least 6 characters
        return password.length() >= 6;
    }

    std::expected<UserSession, std::string> Auth::loginUser(
        std::string_view username,
        std::string_view password
    ) {
        if (username.empty() || password.empty()) {
            return std::unexpected("Username and password cannot be empty.");
        }

        // Query: SELECT user_id, username, password, roles FROM USERS WHERE username = ?
        std::string query = "SELECT user_id, username, password, roles FROM USERS WHERE username = '" 
                        + std::string(username) + "';";

        auto result = database::DatabaseManager::getInstance().executeQuery(query);

        if (!result) {
            return std::unexpected(result.error());
        }

        sql::ResultSet* rs = result.value();

        if (rs->next()) {
            std::string storedPassword = rs->getString("password");
            int userid = rs->getInt("user_id");
            std::string rolesStr = rs->getString("roles");

            // Simple password validation (in production, use hashing!)
            if (storedPassword == std::string(password)) {
                UserSession session;
                session.userid = userid;
                session.username = username;

                // Parse roles (comma-separated)
                if (!rolesStr.empty()) {
                    size_t pos = 0;
                    while ((pos = rolesStr.find(',')) != std::string::npos) {
                        session.roles.push_back(rolesStr.substr(0, pos));
                        rolesStr.erase(0, pos + 1);
                    }
                    session.roles.push_back(rolesStr);
                }

                delete rs;
                return session;
            } else {
                delete rs;
                return std::unexpected("Invalid password.");
            }
        } else {
            delete rs;
            return std::unexpected("User not found.");
        }
    }

    std::expected<int, std::string> Auth::registerUser(
        std::string_view username,
        std::string_view password,
        std::string_view role
    ) {
        if (username.empty() || password.empty()) {
            return std::unexpected("Username and password are required.");
        }

        if (!validatePassword(password)) {
            return std::unexpected("Password must be at least 6 characters.");
        }

        // Check if username already exists
        std::string checkQuery = "SELECT user_id FROM USERS WHERE username = '" 
                                + std::string(username) + "';";
        auto checkResult = database::DatabaseManager::getInstance().executeQuery(checkQuery);
        
        if (checkResult && checkResult.value()->next()) {
            delete checkResult.value();
            return std::unexpected("Username already exists.");
        }
        if (checkResult && checkResult.value()) {
            delete checkResult.value();
        }

        // Insert new user with specified role
        std::string insertQuery = 
            "INSERT INTO USERS (username, password, roles) VALUES ('" +
            std::string(username) + "', '" +
            std::string(password) + "', '" + std::string(role) +"');";

        auto insertResult = database::DatabaseManager::getInstance().executeUpdate(insertQuery);

        if (!insertResult) {
            return std::unexpected(insertResult.error());
        }

        // Query to get the new user_id
        std::string selectQuery = "SELECT user_id FROM USERS WHERE username = '" 
                                + std::string(username) + "';";
        auto selectResult = database::DatabaseManager::getInstance().executeQuery(selectQuery);

        if (!selectResult) {
            return std::unexpected(selectResult.error());
        }

        sql::ResultSet* rs = selectResult.value();
        if (rs->next()) {
            int newUserId = rs->getInt("user_id");
            delete rs;
            return newUserId;
        } else {
            delete rs;
            return std::unexpected("Failed to retrieve new user ID.");
        }
    }

}
