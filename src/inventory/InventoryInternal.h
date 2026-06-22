#pragma once
#include "identity/Auth/Auth.h"
#include "inventory/Apparel/Apparel.h"
#include <string>

namespace auth = ::identity::auth;
namespace apparel = ::inventory::apparel;

namespace inventory::ui {
    bool processRentalCheckout(
        const auth::UserSession& session, 
        const apparel::ApparelCatalog& item, 
        int catalog_id
    );
    
    void showItemDetails(
        const auth::UserSession& session, 
        const std::string &itemIdStr
    );
}