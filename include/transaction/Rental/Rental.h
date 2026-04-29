
#pragma once

#include <string>

struct Rental {
	int rental_id; // Primary Key
	int staff_id;  // Foreign Key -> Staff.staff_id
	int cust_id;   // Foreign Key -> Customers.cust_id
	std::string rental_date;
	std::string expected_return_date;
	double security_deposit;
	double total_fee;
	std::string payment_status;
};

