#include <print>
#include <string>
#include <string_view>
#include <expected>
#include <thread>
#include <chrono>
#include <iostream>

#include "DatabaseManager/DatabaseManager.h"
#include "identity/Auth/Auth.h"
#include "identity/AuthUI.h"
#include "identity/AdminUI.h"
#include "identity/StaffUI.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"

using namespace std;

int main() {

    if (!database::DatabaseManager::getInstance().connect()) {
        println(stderr, "Application failed to start: Database connection error.");
        return 1;
    }

// 2. Instantiate Auth Service
    ::identity::auth::Auth authService;
    
    // Formal Landing Screen
    ::identity::authui::showSplashScreen();

    bool systemRunning = true;
    while (systemRunning) {
        println("\n--- MAIN GATEWAY ---");
        println("[1] Login");
        println("[2] Register New Account");
        println("[3] Exit System");
        print("Selection: ");

        int choice;
        // Basic input validation for CLI navigation
        if (!(cin >> choice)) {
            cin.clear();
            cin.ignore(1000, '\n');
            println("Invalid input. Please enter a number.");
            continue;
        }

        switch (choice) {
            case 1:
            {
                auto sessionResult = authService.handleLoginFlow();
                if (sessionResult) {
                    auto& session = sessionResult.value();
                    
                    // Route to appropriate dashboard based on role
                    std::string role = session.roles.front();
                    
                    if (role == "admin") {
                        ::identity::adminui::handleAdminDashboard(session);
                    } else if (role == "staff") {
                        ::identity::staffui::handleStaffDashboard(session);
                    } else {
                        // Default to customer dashboard
                        ::identity::authui::handleCustomerDashboard(session);
                    }
                }
                break;
            }
                
            case 2:
                // handleRegisterFlow handles all registration inputs and DB logic
                authService.handleRegisterFlow("customer");
                break;
            case 3:
                println("Shutting down FWCRS. Goodbye.");
                systemRunning = false;
                break;
            default:
                println("Invalid selection. Please try again.");
        }
    }

    // --- ADDED END ---

    return 0;
}

