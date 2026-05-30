#pragma once
#include "identity/Auth/Auth.h"

namespace identity::staffui {
    // Staff Dashboard Functions
    void showStaffDashboard(const ::identity::auth::UserSession& session);
    void handleStaffDashboard(const ::identity::auth::UserSession& session);
    
    // Staff menu options
    void manageApparelInventory(const ::identity::auth::UserSession& session);
    void processApparelReturn();
    void viewActiveRentals();
    void viewStaffProfile(const ::identity::auth::UserSession& session);
}
