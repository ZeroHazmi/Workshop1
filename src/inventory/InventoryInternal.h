#pragma once
#include "identity/Auth/Auth.h"
#include "inventory/Apparel/Apparel.h"
#include <string>

namespace inventory::ui {
    bool processRentalCheckout(
        const ::identity::auth::UserSession& session, 
        const inventory::apparel::ApparelCatalog& item, 
        int catalog_id
    );
    
    void showItemDetails(
        const ::identity::auth::UserSession& session, 
        const std::string &itemIdStr
    );
}
