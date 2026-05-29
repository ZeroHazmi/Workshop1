#pragma once
#include "inventory/InventoryUI.h"
#include "identity/Auth/Auth.h"

using namespace std;

namespace inventory::ui {
    void showCatalog(const ::identity::auth::UserSession& session);
}