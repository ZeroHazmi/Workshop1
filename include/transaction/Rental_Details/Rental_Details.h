#pragma once

#include <string>

struct RentalDetail {
	int detail_id; // Primary Key
	int rental_id; // Foreign Key -> rental.rental_id
	int item_id; // Foreign Key -> apparel_item.item_id
	double subtotal;
	std::string actual_return_date;
	double late_fee;
	std::string return_condition;
};

