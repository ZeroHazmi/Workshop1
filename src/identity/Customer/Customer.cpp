#include "identity/Customer/Customer.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>

namespace identity::customer {
    
    std::expected<int, std::string> createCustomerProfile(
        int userId,
        std::string_view fullname,
        std::string_view email,
        std::string_view phone
    ) {
        if (fullname.empty()) {
            return std::unexpected("Full name is required for customer profile.");
        }

        // Insert corresponding CUSTOMER record
        std::string customerQuery =
            "INSERT INTO CUSTOMERS (user_id, fullname, email, phone_no) VALUES (" +
            std::to_string(userId) + ", '" +
            std::string(fullname) + "', '" +
            std::string(email) + "', '" +
            std::string(phone) + "');";

        auto customerResult = database::DatabaseManager::getInstance().executeUpdate(customerQuery);

        if (customerResult) {
            return userId;
        } else {
            return std::unexpected(customerResult.error());
        }
    }
}
