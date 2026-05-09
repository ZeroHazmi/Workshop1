#pragma once
#include "identity/Auth/Auth.h"

namespace identity::staffui {
    // Staff Dashboard Functions
    void showStaffDashboard(const ::identity::auth::UserSession& session);
    void handleStaffDashboard(const ::identity::auth::UserSession& session);
    
    // Staff menu options
    void viewRentalApparelList();
    void modifyRentalDetails();
    void viewStaffProfile(const ::identity::auth::UserSession& session);
}
