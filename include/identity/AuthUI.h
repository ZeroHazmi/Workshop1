#pragma once
#include "identity/Auth/Auth.h"

namespace identity::authui {
    void showCustomerDashboard(const ::identity::auth::UserSession& session);
    void handleCustomerDashboard(const ::identity::auth::UserSession& session);
    void showAdminDashboard(const ::identity::auth::UserSession& session);
    void showSplashScreen();
    void viewProfile(const ::identity::auth::UserSession& session);
    void manageBankAccount(const ::identity::auth::UserSession& session);
}