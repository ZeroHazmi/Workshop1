#include "identity/Auth/Auth.h"
#include "DatabaseManager/DatabaseManager.h"
#include "identity/Customer/Customer.h"
#include "identity/Staff/Staff.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include "tool/EnvHelper.h"
#include <chrono>
#include <conio.h>
#include <format>
#include <iostream>
#include <limits>
#include <print>
#include <random>
#include <thread>


namespace db = ::database;

using namespace std;

namespace identity::auth {

string Auth::getMaskedPassword() {
  string password;
  char ch;

  while ((ch = _getch()) != '\r') { // '\r' is the Enter key in conio
    if (ch == '\b') {               // Handle Backspace
      if (!password.empty()) {
        password.pop_back();
        print(
            "\b \b"); // Move cursor back, overwrite with space, move back again
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
    this_thread::sleep_for(chrono::milliseconds(1000));
    return;
  }

  int userId = userResult.value();

  string userRole = string(role); // Default role

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
    auto profileResult = ::identity::customer::createCustomerProfile(
        userId, fullname, email, phone);

    if (profileResult) {
      println("\nCustomer registration successful! You can now login.");
    } else {
      println("\nProfile creation failed: {}", profileResult.error());
      string cleanupQuery = "DELETE FROM USERS WHERE user_id = " + to_string(userId) + ";";
      static_cast<void>(db::DatabaseManager::getInstance().executeUpdate(cleanupQuery));
    }
  } else if (userRole == "staff") {
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
    auto profileResult = ::identity::staff::createStaffProfile(
        userId, shopId, staffName, position, phone);

    if (profileResult) {
      println("\nStaff registration successful! You can now login.");
    } else {
      println("\nProfile creation failed: {}", profileResult.error());
      string cleanupQuery = "DELETE FROM USERS WHERE user_id = " + to_string(userId) + ";";
      static_cast<void>(db::DatabaseManager::getInstance().executeUpdate(cleanupQuery));
    }
  } else {
    println("\nUnknown role: {}. Please contact administrator.", userRole);
  }
  this_thread::sleep_for(chrono::milliseconds(1000));
  tool::helper::clearScreen();
}

bool Auth::validatePassword(string_view password) {
  return password.length() >= 6;
}

expected<UserSession, string> Auth::loginUser(string_view username,
                                              string_view password) {
  if (username.empty() || password.empty()) {
    return unexpected("Username and password cannot be empty.");
  }

  string query = "SELECT user_id, username, password, roles FROM USERS WHERE "
                 "username = '" +
                 string(username) + "' AND is_deleted = 0;";

  auto result = db::DatabaseManager::getInstance().executeQuery(query);

  if (!result) {
    return unexpected(result.error());
  }

  sql::ResultSet *rs = result.value();

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

expected<int, string> Auth::registerUser(string_view username,
                                         string_view password,
                                         string_view role) {
  if (username.empty() || password.empty()) {
    return unexpected("Username and password are required.");
  }

  if (!validatePassword(password)) {
    return unexpected("Password must be at least 6 characters.");
  }

  // Check if username already exists
  string checkQuery =
      "SELECT user_id FROM USERS WHERE username = '" + string(username) + "';";
  auto checkResult =
      db::DatabaseManager::getInstance().executeQuery(checkQuery);

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
      string(username) + "', '" + string(password) + "', '" + string(role) +
      "');";

  auto insertResult =
      db::DatabaseManager::getInstance().executeUpdate(insertQuery);

  if (!insertResult) {
    return unexpected(insertResult.error());
  }

  // Query to get the new user_id
  string selectQuery =
      "SELECT user_id FROM USERS WHERE username = '" + string(username) + "';";
  auto selectResult =
      db::DatabaseManager::getInstance().executeQuery(selectQuery);

  if (!selectResult) {
    return unexpected(selectResult.error());
  }

  sql::ResultSet *rs = selectResult.value();
  if (rs->next()) {
    int newUserId = rs->getInt("user_id");
    delete rs;
    return newUserId;
  } else {
    delete rs;
    return unexpected("Failed to retrieve new user ID.");
  }
}

string Auth::getForgotPasswordHtmlBody(const string &pin) {
  return "<div style=''font-family: sans-serif; background-color: #f9f9f9; padding: 30px; border-radius: 8px;''>"
         "  <div style=''max-width: 500px; margin: 0 auto; background-color: #ffffff; padding: 25px; border-radius: 8px; border: 1px solid #eeeeee; box-shadow: 0 4px 8px rgba(0,0,0,0.05);''>"
         "    <h2 style=''color: #333333; margin-top: 0; border-bottom: 2px solid #556cd6; padding-bottom: 10px;''>FWCRS Verification</h2>"
         "    <p style=''color: #555555; font-size: 15px; line-height: 1.5;''>Hello,</p>"
         "    <p style=''color: #555555; font-size: 15px; line-height: 1.5;''>You have requested to reset your password for the <strong>Fancy Wear Costume Rental System (FWCRS)</strong>.</p>"
         "    <p style=''color: #555555; font-size: 15px; line-height: 1.5;''>Please enter the following 6-digit PIN code to verify your identity:</p>"
         "    <div style=''text-align: center; margin: 25px 0;''>"
         "      <span style=''background-color: #f0f4ff; color: #556cd6; font-size: 32px; font-weight: bold; letter-spacing: 5px; padding: 10px 25px; border-radius: 6px; border: 1px dashed #556cd6; display: inline-block;''>" + pin + "</span>"
         "    </div>"
         "    <p style=''color: #888888; font-size: 13px; line-height: 1.4; border-top: 1px solid #eeeeee; padding-top: 15px; margin-bottom: 0;''>If you did not request this, please ignore this email.</p>"
         "  </div>"
         "</div>";
}

bool Auth::sendRealEmail(const string &recipientEmail, const string &pin) {
  string smtpSender = tool::env::get("SMTP_SENDER_EMAIL", "your-email@gmail.com");
  string smtpPassword = tool::env::get("SMTP_PASSWORD", "your-app-password");
  string smtpServer = tool::env::get("SMTP_SERVER", "smtp.gmail.com");
  int smtpPort = 587;
  try {
    smtpPort = stoi(tool::env::get("SMTP_PORT", "587"));
  } catch (...) {}
  bool smtpUseSsl = (tool::env::get("SMTP_USE_SSL", "true") == "true");

  if (smtpSender == "your-email@gmail.com" ||
      smtpPassword == "your-app-password") {
    println("\n  [Notice] SMTP configuration placeholders have not been "
            "populated yet.");
    println("           The email would be sent to: {}", recipientEmail);
    println("           Your 6-digit verification code is: {}", pin);
    return false;
  }

  string htmlBody = Auth::getForgotPasswordHtmlBody(pin);

  // Build the PowerShell command string to run Send-MailMessage
  string psCommand =
      "powershell -NoProfile -WindowStyle Hidden -Command \""
      "[Net.ServicePointManager]::SecurityProtocol = "
      "[Net.SecurityProtocolType]::Tls12; "
      "$SecPass = ConvertTo-SecureString '" +
      smtpPassword +
      "' -AsPlainText -Force; "
      "$Cred = New-Object System.Management.Automation.PSCredential ('" +
      smtpSender +
      "', $SecPass); "
      "Send-MailMessage -To '" +
      recipientEmail + "' -From '" + smtpSender +
      "' "
      "-Subject 'FWCRS Password Reset Verification Code' "
      "-Body '" + htmlBody + "' -BodyAsHtml "
      "-SmtpServer '" +
      smtpServer + "' -Port " + to_string(smtpPort) +
      (smtpUseSsl ? " -UseSsl" : "") +
      " -Credential $Cred\"";

  int result = system(psCommand.c_str());
  return (result == 0);
}

string Auth::retrieveEmail(int userId, const string &username, const string &role) {
  auto &db = db::DatabaseManager::getInstance();
  string userEmail;
  if (role == "customer") {
    string emailQuery =
        "SELECT email FROM CUSTOMERS WHERE user_id = " + to_string(userId) +
        " AND is_deleted = 0;";
    auto emailRes = db.executeQuery(emailQuery);
    if (emailRes) {
      sql::ResultSet *ers = emailRes.value();
      if (ers->next()) {
        userEmail = ers->getString("email");
      }
      delete ers;
    }
  }
  return userEmail;
}

bool Auth::runPinVerificationLoop(const string &userEmail, string &currentPin) {
  auto generatePin = []() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(100000, 999999);
    return to_string(dis(gen));
  };

  auto dispatchEmail = [](const string &email, const string &pin) {
    println("\n  Sending verification PIN to {}...", email);
    bool emailSent = Auth::sendRealEmail(email, pin);
    if (emailSent) {
      println("  Success: Real email sent to {} via background process.",
              email);
    }
  };

  dispatchEmail(userEmail, currentPin);

  while (true) {
    println("\n  [1] Enter Verification PIN");
    println("  [2] Resend Verification PIN");
    println("  [0] Cancel");
    print("  Select option: ");

    int choice;
    if (!(cin >> choice)) {
      cin.clear();
      cin.ignore(numeric_limits<streamsize>::max(), '\n');
      println("  Invalid option.");
      this_thread::sleep_for(chrono::milliseconds(1200));
      tool::helper::clearScreen();
      continue;
    }
    cin.ignore(numeric_limits<streamsize>::max(), '\n');

    if (choice == 0) {
      println("  Recovery cancelled.");
      this_thread::sleep_for(chrono::milliseconds(1000));
      return false;
    } else if (choice == 2) {
      currentPin = generatePin();
      dispatchEmail(userEmail, currentPin);
      continue;
    } else if (choice == 1) {
      print("  Enter the 6-digit PIN: ");
      string enteredPin;
      cin >> enteredPin;
      cin.ignore(numeric_limits<streamsize>::max(), '\n');

      if (enteredPin != currentPin) {
        println("  [Error] Invalid verification PIN. Please try again.");
        this_thread::sleep_for(chrono::milliseconds(1200));
        tool::helper::clearScreen();
        continue;
      }

      return true;
    } else {
      println("  Invalid option.");
      this_thread::sleep_for(chrono::milliseconds(1200));
      tool::helper::clearScreen();
    }
  }
}

void Auth::runPasswordResetLoop(int userId) {
  auto &db = db::DatabaseManager::getInstance();
  bool updatingPassword = true;
  while (updatingPassword) {
    println("\n  --- RESET PASSWORD ---");
    print("  Enter New Password (min 6 characters): ");
    string newPassword = getMaskedPassword();

    print("  Confirm New Password: ");
    string confirmPassword = getMaskedPassword();

    if (!validatePassword(newPassword)) {
      println("  [Error] Password must be at least 6 characters.");
      this_thread::sleep_for(chrono::milliseconds(1200));
      continue;
    }

    if (newPassword != confirmPassword) {
      println("  [Error] Passwords do not match. Please try again.");
      this_thread::sleep_for(chrono::milliseconds(1200));
      continue;
    }

    string updateQuery = "UPDATE USERS SET password = '" + newPassword +
                         "' WHERE user_id = " + to_string(userId) + ";";
    auto updateRes = db.executeUpdate(updateQuery);
    if (updateRes) {
      println("\n  Success: Your password has been reset successfully. You can "
              "now login.");
      updatingPassword = false;
    } else {
      cout << "  [Error] Failed to update password in database: "
           << updateRes.error() << "\n";
      updatingPassword = false;
    }
  }
}

std::optional<RecoveryUser> Auth::verifyUsernameExists(const string &usernameInput) {
  auto &db = db::DatabaseManager::getInstance();
  string checkUserQuery =
      "SELECT user_id, username, roles FROM USERS WHERE username = '" +
      usernameInput + "' AND is_deleted = 0;";
  auto checkUserRes = db.executeQuery(checkUserQuery);
  if (!checkUserRes) {
    cout << "  [Error] Database error: " << checkUserRes.error() << "\n";
    this_thread::sleep_for(chrono::milliseconds(1500));
    return nullopt;
  }

  sql::ResultSet *urs = checkUserRes.value();
  if (!urs->next()) {
    delete urs;
    println("  [Error] Username not found.");
    this_thread::sleep_for(chrono::milliseconds(1500));
    return nullopt;
  }

  int userId = urs->getInt("user_id");
  string dbUsername = urs->getString("username");
  string rolesStr = urs->getString("roles");
  delete urs;

  string role = "customer";
  if (!rolesStr.empty()) {
    size_t pos = rolesStr.find(',');
    if (pos != string::npos) {
      role = rolesStr.substr(0, pos);
    } else {
      role = rolesStr;
    }
  }

  return RecoveryUser{userId, dbUsername, role};
}

void Auth::handleForgotPasswordFlow() {
  tool::ui::showHeader("PASSWORD RECOVERY", 64);

  print("  Enter Username: ");
  string usernameInput;
  cin >> usernameInput;
  cin.ignore(numeric_limits<streamsize>::max(), '\n');

  if (usernameInput.empty()) {
    println("  Username cannot be empty.");
    this_thread::sleep_for(chrono::milliseconds(1200));
    return;
  }

  auto userOpt = Auth::verifyUsernameExists(usernameInput);
  if (!userOpt) {
    return;
  }

  string userEmail = Auth::retrieveEmail(userOpt->userId, userOpt->username, userOpt->role);
  if (userEmail.empty()) {
    println("  [Error] No registered email found for this user.");
    this_thread::sleep_for(chrono::milliseconds(1500));
    return;
  }

  auto generatePin = []() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dis(100000, 999999);
    return to_string(dis(gen));
  };

  string currentPin = generatePin();

  if (Auth::runPinVerificationLoop(userEmail, currentPin)) {
    Auth::runPasswordResetLoop(userOpt->userId);
  }

  this_thread::sleep_for(chrono::milliseconds(1500));
}

} // namespace identity::auth