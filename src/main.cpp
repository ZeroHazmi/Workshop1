#include <print>
#include <string>
#include <string_view>
#include <expected>
#include <thread>
#include <chrono>
#include <iostream>

#ifdef _WIN32
#include <windows.h>
#endif

#include "DatabaseManager/DatabaseManager.h"
#include "identity/Auth/Auth.h"
#include "identity/AuthUI.h"
#include "identity/AdminUI.h"
#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"

namespace db = ::database;
namespace auth = ::identity::auth;
namespace authui = ::identity::authui;
namespace adminui = ::identity::adminui;
namespace staffui = ::identity::staffui;

using namespace std;

int main() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    if (!db::DatabaseManager::getInstance().connect()) {
        println(stderr, "Application failed to start: Database connection error.");
        return 1;
    }

// 2. Instantiate Auth Service
    auth::Auth authService;
    
    // Formal Landing Screen
    authui::showSplashScreen();

    int invalidAttempts = 0;
    bool systemRunning = true;
    while (systemRunning) {
        println("\n--- MAIN GATEWAY ---");
        println("[1] Login");
        println("[2] Register New Account");
        println("[3] Exit System");
        print("Selection: ");

        int choice;
        // Basic input validation for CLI navigation
        if (!tool::input::readInt(choice)) {
            println("  Invalid selection.");
            if (tool::ui::handleInvalidAttempt(invalidAttempts)) {
                tool::helper::clearScreen();
                authui::showSplashScreen();
            } else {
                this_thread::sleep_for(chrono::milliseconds(1000));
            }
            continue;
        }

        switch (choice) {
            case 1:
            {
                invalidAttempts = 0;
                auto sessionResult = authService.handleLoginFlow();
                if (sessionResult) {
                    auto& session = sessionResult.value();
                    
                    // Route to appropriate dashboard based on role
                    string role = session.roles.front();
                    
                    if (role == "admin") {
                        adminui::handleAdminDashboard(session);
                    } else if (role == "staff") {
                        staffui::handleStaffDashboard(session);
                    } else {
                        // Default to customer dashboard
                        authui::handleCustomerDashboard(session);
                    }
                } else {
                    tool::helper::clearScreen();
                    authui::showSplashScreen();
                }
                break;
            }
                
            case 2:
                invalidAttempts = 0;
                // handleRegisterFlow handles all registration inputs and DB logic
                authService.handleRegisterFlow("customer");
                break;
            case 3:
                invalidAttempts = 0;
                println("Shutting down FWCRS. Goodbye.");
                systemRunning = false;
                break;
            default:
                println("  Invalid selection.");
                if (tool::ui::handleInvalidAttempt(invalidAttempts)) {
                    tool::helper::clearScreen();
                    authui::showSplashScreen();
                } else {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
        }
    }
    return 0;
}
