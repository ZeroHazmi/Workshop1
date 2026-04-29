
#pragma once

#include <string>
#include <vector>

namespace identify {
	struct User {
		int user_id; // Primary Key
		std::string username;
		std::string password;
		std::vector<std::string> roles;
	};

	struct StaffRecord {
		int staff_id; // Primary Key
		int user_id;  // Foreign Key -> Users.user_id
		int shop_id;  // Foreign Key -> Shops.shop_id
		std::string staff_name;
		std::string position;
		std::string phone_no;
	};

	struct Customer {
		int cust_id; // Primary Key
		int user_id; // Foreign Key -> Users.user_id
		std::string fullname;
		std::string phone_no;
		std::string email;
	};
}

