#pragma once

#include "identity/Auth/Auth.h"

namespace inventory::ui {
    void showCatalog();
    void registerNewApparel(const ::identity::auth::UserSession& session);
}