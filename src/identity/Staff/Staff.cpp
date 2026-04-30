#include "identity/Staff/Staff.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>

namespace identity::staff {
    
    std::expected<int, std::string> createStaffProfile(
        int userId,
        int shopId,
        std::string_view staffName,
        std::string_view position,
        std::string_view phone
    ) {
        if (staffName.empty() || position.empty()) {
            return std::unexpected("Staff name and position are required.");
        }

        if (shopId <= 0) {
            return std::unexpected("Valid shop ID is required.");
        }

        // Insert STAFF record
        std::string staffQuery =
            "INSERT INTO STAFF (user_id, shop_id, staff_name, position, phone_no) VALUES (" +
            std::to_string(userId) + ", " +
            std::to_string(shopId) + ", '" +
            std::string(staffName) + "', '" +
            std::string(position) + "', '" +
            std::string(phone) + "');";

        auto staffResult = database::DatabaseManager::getInstance().executeUpdate(staffQuery);

        if (staffResult) {
            return userId;
        } else {
            return std::unexpected(staffResult.error());
        }
    }
}
