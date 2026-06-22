#pragma once

#include <string>
#include <vector>
#include <expected>
#include <string_view>
#include <optional>

namespace identity::auth {

// Session data structure for logged-in users
struct UserSession {
    int userid;
    std::string username;
    std::vector<std::string> roles; 
};

struct RecoveryUser {
    int userId;
    std::string username;
    std::string role;
};

// Auth class - handles login, register, and splash screen
class Auth {
public:
    Auth() = default;
    ~Auth() = default;

    static std::string getMaskedPassword();
    static std::expected<UserSession, std::string> handleLoginFlow();
    static void handleRegisterFlow(std::string_view role = "customer");
    static void handleForgotPasswordFlow();

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

    // Helpers for Forgot Password flow
    static std::string getForgotPasswordHtmlBody(const std::string& pin);
    static bool sendRealEmail(const std::string& recipientEmail, const std::string& pin);
    static std::string retrieveEmail(int userId, const std::string& username, const std::string& role);
    static bool runPinVerificationLoop(const std::string& userEmail, std::string& currentPin);
    static void runPasswordResetLoop(int userId);
    static std::optional<RecoveryUser> verifyUsernameExists(const std::string& usernameInput);
};

} // namespace identity::auth
