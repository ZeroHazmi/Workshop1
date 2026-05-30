#pragma once
#include "identity/Auth/Auth.h"

namespace identity::adminui {
    // Admin Dashboard Functions
    void showAdminDashboard(const ::identity::auth::UserSession& session);
    void handleAdminDashboard(const ::identity::auth::UserSession& session);
    
    // Admin menu options
    void registerStaffAccount();
    void manageStaffAccount();
    void viewAdminProfile(const ::identity::auth::UserSession& session);
    void registerNewShop();
    void manageShopInformation();
    void displayShopList();
    void viewShopInventory();

    // Submenu gateways
    void showStaffManagementSubmenu(const ::identity::auth::UserSession& session);
    void showShopManagementSubmenu(const ::identity::auth::UserSession& session);
    void showBusinessStatsSubmenu(const ::identity::auth::UserSession& session);
}
