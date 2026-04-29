#include "identity/Profile/Profile.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>
#include <iostream>

namespace identity::profile {

// ========== CUSTOMER PROFILE MANAGEMENT ==========

std::expected<CustomerProfile, std::string> Profile::getCustomerProfile(int user_id) {
	std::string query = "SELECT cust_id, user_id, fullname, email, phone_no FROM CUSTOMERS WHERE user_id = " 
					   + std::to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	if (rs->next()) {
		CustomerProfile customer;
		customer.cust_id = rs->getInt("cust_id");
		customer.user_id = rs->getInt("user_id");
		customer.fullname = rs->getString("fullname");
		customer.email = rs->getString("email");
		customer.phone_no = rs->getString("phone_no");

		delete rs;
		return customer;
	}

	delete rs;
	return std::unexpected("Customer profile not found.");
}

std::expected<bool, std::string> Profile::updateCustomerProfile(
	int user_id,
	std::string_view fullname,
	std::string_view email,
	std::string_view phone_no
) {
	std::string query = "UPDATE CUSTOMERS SET fullname = '" + std::string(fullname) + 
					   "', email = '" + std::string(email) + 
					   "', phone_no = '" + std::string(phone_no) + 
					   "' WHERE user_id = " + std::to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	return true;
}

// ========== STAFF PROFILE MANAGEMENT ==========

std::expected<StaffProfile, std::string> Profile::getStaffProfile(int user_id) {
	std::string query = "SELECT staff_id, user_id, shop_id, staff_name, position, phone_no FROM STAFF WHERE user_id = " 
				   + std::to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	if (rs->next()) {
		StaffProfile staff;
		staff.staff_id = rs->getInt("staff_id");
		staff.user_id = rs->getInt("user_id");
		staff.shop_id = rs->getInt("shop_id");
		staff.staff_name = rs->getString("staff_name");
		staff.position = rs->getString("position");
		staff.phone_no = rs->getString("phone_no");

		delete rs;
		return staff;
	}

	delete rs;
	return std::unexpected("Staff profile not found.");
}

std::expected<bool, std::string> Profile::updateStaffProfile(
	int staff_id,
	std::string_view staff_name,
	std::string_view position,
	std::string_view phone_no
) {
	std::string query = "UPDATE STAFF SET staff_name = '" + std::string(staff_name) + 
					   "', position = '" + std::string(position) + 
					   "', phone_no = '" + std::string(phone_no) + 
					   "' WHERE staff_id = " + std::to_string(staff_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	return true;
}

// ========== BANK ACCOUNT MANAGEMENT ==========

std::expected<BankAccount, std::string> Profile::getBankAccount(int user_id) {
	std::string query = "SELECT acc_id, user_id, bank_name, acc_number, acc_holder FROM BANK_ACCOUNT WHERE user_id = " 
				   + std::to_string(user_id) + " LIMIT 1;";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	if (rs->next()) {
		BankAccount account;
		account.acc_id = rs->getInt("acc_id");
		account.user_id = rs->getInt("user_id");
		account.bank_name = rs->getString("bank_name");
		account.acc_number = rs->getString("acc_number");
		account.acc_holder = rs->getString("acc_holder");

		delete rs;
		return account;
	}

	delete rs;
	return std::unexpected("Bank account not found.");
}

std::expected<std::vector<BankAccount>, std::string> Profile::getAllBankAccounts(int user_id) {
	std::vector<BankAccount> accounts;
	std::string query = "SELECT acc_id, user_id, bank_name, acc_number, acc_holder FROM BANK_ACCOUNT WHERE user_id = " 
					   + std::to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	while (rs->next()) {
		BankAccount account;
		account.acc_id = rs->getInt("acc_id");
		account.user_id = rs->getInt("user_id");
		account.bank_name = rs->getString("bank_name");
		account.acc_number = rs->getString("acc_number");
		account.acc_holder = rs->getString("acc_holder");

		accounts.push_back(account);
	}

	delete rs;
	return accounts;
}

std::expected<int, std::string> Profile::linkBankAccount(
	int user_id,
	std::string_view bank_name,
	std::string_view acc_number,
	std::string_view acc_holder
) {
	// Check if account already exists for this user
	auto existingAccount = getBankAccount(user_id);
	if (existingAccount) {
		return std::unexpected("User already has a bank account linked.");
	}

	std::string query = "INSERT INTO BANK_ACCOUNT (user_id, bank_name, acc_number, acc_holder) VALUES (" +
					   std::to_string(user_id) + ", '" +
					   std::string(bank_name) + "', '" +
					   std::string(acc_number) + "', '" +
					   std::string(acc_holder) + "');";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	// Retrieve the new account ID
	std::string selectQuery = "SELECT acc_id FROM BANK_ACCOUNT WHERE user_id = " + std::to_string(user_id) + ";";
	auto selectResult = database::DatabaseManager::getInstance().executeQuery(selectQuery);

	if (!selectResult) {
		return std::unexpected(selectResult.error());
	}

	sql::ResultSet* rs = selectResult.value();
	if (rs->next()) {
		int newAccId = rs->getInt("acc_id");
		delete rs;
		return newAccId;
	}

	delete rs;
	return std::unexpected("Failed to retrieve new account ID.");
}

std::expected<bool, std::string> Profile::updateBankAccount(
	int acc_id,
	std::string_view bank_name,
	std::string_view acc_number,
	std::string_view acc_holder
) {
	std::string query = "UPDATE BANK_ACCOUNT SET bank_name = '" + std::string(bank_name) + 
					   "', acc_number = '" + std::string(acc_number) + 
					   "', acc_holder = '" + std::string(acc_holder) + 
					   "' WHERE acc_id = " + std::to_string(acc_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	return true;
}

std::expected<bool, std::string> Profile::removeBankAccount(int acc_id) {
	std::string query = "DELETE FROM BANK_ACCOUNT WHERE acc_id = " + std::to_string(acc_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return std::unexpected(result.error());
	}

	return true;
}

} // namespace identity::profile
