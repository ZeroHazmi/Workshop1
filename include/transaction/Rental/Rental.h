
#pragma once

#include <string>
#include <expected>

struct Rental {
	int rental_id; // Primary Key
	int shop_id;   // Foreign Key -> shops.shop_id
	int cust_id;   // Foreign Key -> customers.cust_id
	std::string rental_date;
	std::string expected_return_date;
	double security_deposit;
	double total_fee;
	std::string payment_status;
};

namespace transaction::rental {
    std::expected<std::string, std::string> createRental(
        int user_id, 
        int catalog_id, 
        const std::string& size, 
        const std::string& start_date, 
        const std::string& expected_return_date, 
        double daily_rate, 
        int total_days
    );
}

