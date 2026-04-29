#include "identity/Auth/Auth.h"
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
    void Auth::handleRegisterFlow() {
        std::string user, pass, name, email, phone;
        
        std::println("\n--- NEW CUSTOMER REGISTRATION ---");
        std::print("Username: "); std::cin >> user;
        std::print("Password: ");
        pass = getMaskedPassword();
        
        // Clear buffer and get full name
        std::print("Full Name: ");
        std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
        std::getline(std::cin, name);
        
        std::print("Email: "); std::cin >> email;
        std::print("Phone: "); std::cin >> phone;

        auto result = registerUser(user, pass, name, email, phone);
        
        if (result) {
            std::println("\nRegistration successful! You can now login.");
        } else {
            std::println("\nRegistration failed: {}", result.error());
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
        std::string_view fullname,
        std::string_view email,
        std::string_view phone,
        std::string_view role
    ) {
        if (username.empty() || password.empty() || fullname.empty()) {
            return std::unexpected("Username, password, and fullname are required.");
        }

        if (!validatePassword(password)) {
            return std::unexpected("Password must be at least 6 characters.");
        }

        // First, check if username already exists
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

        // Insert new user with role "customer"
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

            // Insert corresponding CUSTOMER record
            std::string customerQuery =
                "INSERT INTO CUSTOMERS (user_id, fullname, email, phone_no) VALUES (" +
                std::to_string(newUserId) + ", '" +
                std::string(fullname) + "', '" +
                std::string(email) + "', '" +
                std::string(phone) + "');";

            auto customerResult = database::DatabaseManager::getInstance().executeUpdate(customerQuery);
            delete rs;

            if (customerResult) {
                return newUserId;
            } else {
                return std::unexpected(customerResult.error());
            }
        } else {
            delete rs;
            return std::unexpected("Failed to retrieve new user ID.");
        }
    }

}
