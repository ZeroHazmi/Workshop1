#pragma once

#include <string>
#include <vector>
#include <expected>
#include <string_view>

namespace identity::auth {

// Session data structure for logged-in users
struct UserSession {
    int userid;
    std::string username;
    std::vector<std::string> roles;  // Can contain "admin", "staff", "customer"
};

// Auth class - handles login, register, and splash screen
class Auth {
public:
    Auth() = default;
    ~Auth() = default;

    static std::string getMaskedPassword();
    static std::expected<UserSession, std::string> handleLoginFlow();
    static void handleRegisterFlow(std::string_view role = "customer");

    // Login user with credentials
    static std::expected<UserSession, std::string> loginUser(
        std::string_view username,
        std::string_view password
    );

    // Register new user (creates user account only, profile created separately)
    static std::expected<int, std::string> registerUser(
        std::string_view username,
        std::string_view password,
        std::string_view role = "customer"
    );

private:
    // Helper to validate password (basic check)
    static bool validatePassword(std::string_view password);
};

} // namespace identity::auth
