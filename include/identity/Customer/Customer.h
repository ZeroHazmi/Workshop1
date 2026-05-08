#pragma once
#include <string>
#include <string_view>
#include <expected>

namespace identity::customer {
    // Create a customer profile for a newly registered user
    std::expected<int, std::string> createCustomerProfile(
        int userId,
        std::string_view fullname,
        std::string_view email,
        std::string_view phone
    );
}
