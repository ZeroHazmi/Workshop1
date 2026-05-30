
#pragma once

#include <string>

struct BankAccount {
	int acc_id;   // Primary Key
	int user_id;  // Foreign Key -> Users.user_id
	std::string bank_name;
	std::string acc_number;
	std::string acc_holder;
};

