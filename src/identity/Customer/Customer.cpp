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
        if (email.empty()) {
            return unexpected("Email is required for customer profile.");
        }

        // Check if email already exists
        string checkEmailQuery = "SELECT customer_id FROM CUSTOMERS WHERE email = '" + string(email) + "' AND is_deleted = 0;";
        auto checkRes = db::DatabaseManager::getInstance().executeQuery(checkEmailQuery);
        if (checkRes && checkRes.value()->next()) {
            delete checkRes.value();
            return unexpected("Email address is already registered.");
        }
        if (checkRes && checkRes.value()) {
            delete checkRes.value();
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