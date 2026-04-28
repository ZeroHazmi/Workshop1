
#pragma once

#include <string>

struct ApparelItem {
	int apparel_id; // Primary Key
	int shop_id;    // Foreign Key -> Shops.shop_id
	std::string description;
	std::string category;
	double daily_rate;
	std::string condition_status;
	std::string status; // e.g., available/reserved/rented
};

