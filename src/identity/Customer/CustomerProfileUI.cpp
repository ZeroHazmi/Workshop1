#include "identity/AuthUI.h"
#include "tool/CLIComponents.h"
#include "identity/Profile/Profile.h"
#include <print>
#include <string>

using namespace std;

namespace identity::authui {

    void viewProfile(const ::identity::auth::UserSession& session) {
        tool::ui::showHeader("USER PROFILE", 64);

        auto profileOpt = ::identity::profile::Profile::getCustomerProfile(session.userid);
        if (profileOpt) {
            auto& profile = profileOpt.value();
            tool::ui::printField("Username", session.username);
            tool::ui::printField("Full Name", profile.fullname);
            tool::ui::printField("Email", profile.email);
            tool::ui::printField("Phone", profile.phone_no);
        } else {
            println("  Error: {}", profileOpt.error());
            println("  (Please ensure your customer profile has been fully set up.)");
        }
        
        tool::ui::pressZeroToReturn("dashboard", 64);
    }

}
