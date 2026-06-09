#include "identity/Staff/Staff.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>

namespace db = ::database;

using namespace std;

namespace identity::staff {
    
    expected<int, string> createStaffProfile(
        int userId,
        int shopId,
        string_view staffName,
        string_view position,
        string_view phone
    ) {
        if (staffName.empty() || position.empty()) {
            return unexpected("Staff name and position are required.");
        }

        if (shopId <= 0) {
            return unexpected("Valid shop ID is required.");
        }

        // Insert STAFF record
        string staffQuery =
            "INSERT INTO STAFF (user_id, shop_id, staff_name, position, phone_no) VALUES (" +
            to_string(userId) + ", " +
            to_string(shopId) + ", '" +
            string(staffName) + "', '" +
            string(position) + "', '" +
            string(phone) + "');";

        auto staffResult = db::DatabaseManager::getInstance().executeUpdate(staffQuery);

        if (staffResult) {
            return userId;
        } else {
            return unexpected(staffResult.error());
        }
    }
}