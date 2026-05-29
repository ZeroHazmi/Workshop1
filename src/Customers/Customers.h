
#pragma once

#include <string>

struct Customer {
	int cust_id; // Primary Key
	int user_id; // Foreign Key -> Users.user_id
	std::string fullname;
	std::string phone_no;
	std::string email;
};

