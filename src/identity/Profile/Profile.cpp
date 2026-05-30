#include "identity/Profile/Profile.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>
#include <iostream>

using namespace std;

namespace identity::profile {

// ========== CUSTOMER PROFILE MANAGEMENT ==========

expected<CustomerProfile, string> Profile::getCustomerProfile(int user_id) {
	string query = "SELECT cust_id, user_id, fullname, email, phone_no FROM CUSTOMERS WHERE user_id = " 
					   + to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
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
	return unexpected("Customer profile not found.");
}

expected<bool, string> Profile::updateCustomerProfile(
	int user_id,
	string_view fullname,
	string_view email,
	string_view phone_no
) {
	string query = "UPDATE CUSTOMERS SET fullname = '" + string(fullname) + 
					   "', email = '" + string(email) + 
					   "', phone_no = '" + string(phone_no) + 
					   "' WHERE user_id = " + to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

// ========== STAFF PROFILE MANAGEMENT ==========

expected<StaffProfile, string> Profile::getStaffProfile(int user_id) {
	string query = "SELECT staff_id, user_id, shop_id, staff_name, position, phone_no FROM STAFF WHERE user_id = " 
				   + to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
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
	return unexpected("Staff profile not found.");
}

expected<bool, string> Profile::updateStaffProfile(
	int staff_id,
	string_view staff_name,
	string_view position,
	string_view phone_no
) {
	string query = "UPDATE STAFF SET staff_name = '" + string(staff_name) + 
					   "', position = '" + string(position) + 
					   "', phone_no = '" + string(phone_no) + 
					   "' WHERE staff_id = " + to_string(staff_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

// ========== BANK ACCOUNT MANAGEMENT ==========

expected<BankAccount, string> Profile::getBankAccount(int user_id) {
	string query = "SELECT acc_id, user_id, bank_name, acc_number, acc_holder, balance FROM BANK_ACCOUNT WHERE user_id = " 
				   + to_string(user_id) + " AND is_deleted = 0 LIMIT 1;";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	if (rs->next()) {
		BankAccount account;
		account.acc_id = rs->getInt("acc_id");
		account.user_id = rs->getInt("user_id");
		account.bank_name = rs->getString("bank_name");
		account.acc_number = rs->getString("acc_number");
		account.acc_holder = rs->getString("acc_holder");
		account.balance = rs->getDouble("balance");

		delete rs;
		return account;
	}

	delete rs;
	return unexpected("Bank account not found.");
}

expected<vector<BankAccount>, string> Profile::getAllBankAccounts(int user_id) {
	vector<BankAccount> accounts;
	string query = "SELECT acc_id, user_id, bank_name, acc_number, acc_holder, balance FROM BANK_ACCOUNT WHERE user_id = " 
					   + to_string(user_id) + " AND is_deleted = 0;";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	while (rs->next()) {
		BankAccount account;
		account.acc_id = rs->getInt("acc_id");
		account.user_id = rs->getInt("user_id");
		account.bank_name = rs->getString("bank_name");
		account.acc_number = rs->getString("acc_number");
		account.acc_holder = rs->getString("acc_holder");
		account.balance = rs->getDouble("balance");

		accounts.push_back(account);
	}

	delete rs;
	return accounts;
}

expected<int, string> Profile::linkBankAccount(
	int user_id,
	string_view bank_name,
	string_view acc_number,
	string_view acc_holder
) {
	// Check if account already exists for this user
	auto existingAccount = getBankAccount(user_id);
	if (existingAccount) {
		return unexpected("User already has a bank account linked.");
	}

	string query = "INSERT INTO BANK_ACCOUNT (user_id, bank_name, acc_number, acc_holder) VALUES (" +
					   to_string(user_id) + ", '" +
					   string(bank_name) + "', '" +
					   string(acc_number) + "', '" +
					   string(acc_holder) + "');";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	// Retrieve the new account ID
	string selectQuery = "SELECT acc_id FROM BANK_ACCOUNT WHERE user_id = " + to_string(user_id) + " AND is_deleted = 0;";
	auto selectResult = database::DatabaseManager::getInstance().executeQuery(selectQuery);

	if (!selectResult) {
		return unexpected(selectResult.error());
	}

	sql::ResultSet* rs = selectResult.value();
	if (rs->next()) {
		int newAccId = rs->getInt("acc_id");
		delete rs;
		return newAccId;
	}

	delete rs;
	return unexpected("Failed to retrieve new account ID.");
}

expected<bool, string> Profile::updateBankAccount(
	int acc_id,
	string_view bank_name,
	string_view acc_number,
	string_view acc_holder
) {
	string query = "UPDATE BANK_ACCOUNT SET bank_name = '" + string(bank_name) + 
					   "', acc_number = '" + string(acc_number) + 
					   "', acc_holder = '" + string(acc_holder) + 
					   "' WHERE acc_id = " + to_string(acc_id) + " AND is_deleted = 0;";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

expected<bool, string> Profile::removeBankAccount(int acc_id) {
	string query = "UPDATE BANK_ACCOUNT SET is_deleted = 1 WHERE acc_id = " + to_string(acc_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

expected<bool, string> Profile::depositBalance(int acc_id, double amount) {
	string query = "UPDATE BANK_ACCOUNT SET balance = balance + " + to_string(amount) + " WHERE acc_id = " + to_string(acc_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

} // namespace identity::profile
