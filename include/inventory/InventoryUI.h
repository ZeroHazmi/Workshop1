#pragma once
#include "identity/Auth/Auth.h"

namespace inventory::ui {
    void showCatalog(const ::identity::auth::UserSession& session);
    void registerNewApparel(const ::identity::auth::UserSession& session);
}