
#pragma once

#include <string>
#include <vector>
#include <expected>
#include <string_view>

namespace identity::profile {

struct BankAccount {
    int acc_id;   // Primary Key
    std::string unique_id;
    int user_id;  // Foreign Key -> Users.user_id
    std::string bank_name;
    std::string acc_number;
    std::string acc_holder;
    double balance;
};

struct CustomerProfile {
	int cust_id;
	std::string unique_id;
	int user_id;
	std::string fullname;
	std::string phone_no;
	std::string email;
};

struct StaffProfile {
	int staff_id;
	std::string unique_id;
	int user_id;
	int shop_id;
	std::string staff_name;
	std::string position;
	std::string phone_no;
};

class Profile {
public:
	Profile() = default;
	~Profile() = default;

	// ========== CUSTOMER PROFILE MANAGEMENT ==========
	// READ: Get customer profile by user_id
	static std::expected<CustomerProfile, std::string> getCustomerProfile(int user_id);

	// UPDATE: Update customer personal data
	static std::expected<bool, std::string> updateCustomerProfile(
		int user_id,
		std::string_view fullname,
		std::string_view email,
		std::string_view phone_no
	);

	// ========== STAFF PROFILE MANAGEMENT ==========
	// READ: Get staff profile by user_id
	static std::expected<StaffProfile, std::string> getStaffProfile(int user_id);

	// UPDATE: Update staff personal data
	static std::expected<bool, std::string> updateStaffProfile(
		int staff_id,
		std::string_view staff_name,
		std::string_view position,
		std::string_view phone_no
	);

	// ========== BANK ACCOUNT MANAGEMENT ==========
	// READ: Get bank account by user_id
	static std::expected<BankAccount, std::string> getBankAccount(int user_id);

	// READ: Get all bank accounts for a user (in case multiple)
	static std::expected<std::vector<BankAccount>, std::string> getAllBankAccounts(int user_id);

	// CREATE: Link bank account to user
	static std::expected<int, std::string> linkBankAccount(
		int user_id,
		std::string_view bank_name,
		std::string_view acc_number,
		std::string_view acc_holder
	);

	// UPDATE: Update bank account details
	static std::expected<bool, std::string> updateBankAccount(
		int acc_id,
		std::string_view bank_name,
		std::string_view acc_number,
		std::string_view acc_holder
	);

	// DELETE: Remove bank account
	static std::expected<bool, std::string> removeBankAccount(int acc_id);

	// UPDATE: Credit bank balance
	static std::expected<bool, std::string> depositBalance(int acc_id, double amount);

	// ========== UI HELPERS ==========
	void displayCustomerProfileMenu(int user_id);
	void displayBankAccountMenu(int user_id);
};

} // namespace identity::profile
