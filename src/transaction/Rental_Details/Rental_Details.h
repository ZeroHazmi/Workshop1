#pragma once

#include <string>

struct RentalDetail {
	int detail_id; // Primary Key
	int rental_id; // Foreign Key -> Rental.rental_id
	int apparel_id; // Foreign Key -> Apparel.apparel_id
	int quantity;
	double subtotal;
	std::string actual_return_date;
	double late_fee;
	std::string return_condition;
};

