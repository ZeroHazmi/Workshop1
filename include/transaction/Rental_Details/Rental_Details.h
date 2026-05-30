#pragma once

#include <string>

struct RentalDetail {
	int detail_id; // Primary Key
	int rental_id; // Foreign Key -> rental.rental_id
	int item_id; // Foreign Key -> apparel_item.item_id
	std::string actual_return_date;
	std::string return_condition;
	bool is_deleted;
};

