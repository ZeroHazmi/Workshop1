
#pragma once

#include <string>

struct StaffRecord {
	int staff_id; // Primary Key
	int user_id;  // Foreign Key -> Users.user_id
	int shop_id;  // Foreign Key -> Shops.shop_id
	std::string staff_name;
	std::string position;
	std::string phone_no;
};

