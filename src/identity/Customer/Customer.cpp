#include "identity/Customer/Customer.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>

namespace db = ::database;

using namespace std;

namespace identity::customer {
    
    expected<int, string> createCustomerProfile(
        int userId,
        string_view fullname,
        string_view email,
        string_view phone
    ) {
        if (fullname.empty()) {
            return unexpected("Full name is required for customer profile.");
        }

        string uniqueId = db::DatabaseManager::generateUniqueId("CST");

        // Insert corresponding CUSTOMER record
        string customerQuery =
            "INSERT INTO CUSTOMERS (user_id, fullname, email, phone_no, unique_id) VALUES (" +
            to_string(userId) + ", '" +
            string(fullname) + "', '" +
            string(email) + "', '" +
            string(phone) + "', '" +
            uniqueId + "');";

        auto customerResult = db::DatabaseManager::getInstance().executeUpdate(customerQuery);

        if (customerResult) {
            return userId;
        } else {
            return unexpected(customerResult.error());
        }
    }
}