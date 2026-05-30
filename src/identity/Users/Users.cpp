#include "identity/Users/Users.h"
#include "DatabaseManager/DatabaseManager.h"
#include <print>
#include <iostream>

using namespace std;

namespace identity::users {

expected<vector<identify::User>, string> Users::getAllUsers() {
	vector<identify::User> users;
	string query = "SELECT user_id, username, password, roles FROM USERS WHERE is_deleted = 0;";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	while (rs->next()) {
		identify::User user;
		user.user_id = rs->getInt("user_id");
		user.username = rs->getString("username");
		user.password = rs->getString("password");

		// Parse roles
		string rolesStr = rs->getString("roles");
		if (!rolesStr.empty()) {
			size_t pos = 0;
			while ((pos = rolesStr.find(',')) != string::npos) {
				user.roles.push_back(rolesStr.substr(0, pos));
				rolesStr.erase(0, pos + 1);
			}
			user.roles.push_back(rolesStr);
		}

		users.push_back(user);
	}

	delete rs;
	return users;
}

expected<identify::User, string> Users::getUserById(int user_id) {
	string query = "SELECT user_id, username, password, roles FROM USERS WHERE user_id = " 
					   + to_string(user_id) + " AND is_deleted = 0;";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	if (rs->next()) {
		identify::User user;
		user.user_id = rs->getInt("user_id");
		user.username = rs->getString("username");
		user.password = rs->getString("password");

		string rolesStr = rs->getString("roles");
		if (!rolesStr.empty()) {
			size_t pos = 0;
			while ((pos = rolesStr.find(',')) != string::npos) {
				user.roles.push_back(rolesStr.substr(0, pos));
				rolesStr.erase(0, pos + 1);
			}
			user.roles.push_back(rolesStr);
		}

		delete rs;
		return user;
	}

	delete rs;
	return unexpected("User not found.");
}

expected<identify::User, string> Users::getUserByUsername(string_view username) {
	string query = "SELECT user_id, username, password, roles FROM USERS WHERE username = '" 
					   + string(username) + "' AND is_deleted = 0;";

	auto result = database::DatabaseManager::getInstance().executeQuery(query);

	if (!result) {
		return unexpected(result.error());
	}

	sql::ResultSet* rs = result.value();
	if (rs->next()) {
		identify::User user;
		user.user_id = rs->getInt("user_id");
		user.username = rs->getString("username");
		user.password = rs->getString("password");

		string rolesStr = rs->getString("roles");
		if (!rolesStr.empty()) {
			size_t pos = 0;
			while ((pos = rolesStr.find(',')) != string::npos) {
				user.roles.push_back(rolesStr.substr(0, pos));
				rolesStr.erase(0, pos + 1);
			}
			user.roles.push_back(rolesStr);
		}

		delete rs;
		return user;
	}

	delete rs;
	return unexpected("User not found.");
}

expected<int, string> Users::createStaffUser(
	string_view username,
	string_view password,
	string_view staff_name,
	string_view position,
	string_view phone_no,
	int shop_id
) {
	// Insert user with role "staff"
	string insertUserQuery =
		"INSERT INTO USERS (username, password, roles) VALUES ('" +
		string(username) + "', '" +
		string(password) + "', 'staff');";

	auto userResult = database::DatabaseManager::getInstance().executeUpdate(insertUserQuery);

	if (!userResult) {
		return unexpected(userResult.error());
	}

	// Get the new user_id
	string selectQuery = "SELECT user_id FROM USERS WHERE username = '" 
							 + string(username) + "';";
	auto selectResult = database::DatabaseManager::getInstance().executeQuery(selectQuery);

	if (!selectResult) {
		return unexpected(selectResult.error());
	}

	sql::ResultSet* rs = selectResult.value();
	if (rs->next()) {
		int newUserId = rs->getInt("user_id");

		// Insert STAFF record
		string insertStaffQuery =
			"INSERT INTO STAFF (user_id, shop_id, staff_name, position, phone_no) VALUES (" +
			to_string(newUserId) + ", " +
			to_string(shop_id) + ", '" +
			string(staff_name) + "', '" +
			string(position) + "', '" +
			string(phone_no) + "');";

		auto staffResult = database::DatabaseManager::getInstance().executeUpdate(insertStaffQuery);
		delete rs;

		if (staffResult) {
			return newUserId;
		} else {
			return unexpected(staffResult.error());
		}
	}

	delete rs;
	return unexpected("Failed to create staff user.");
}

expected<bool, string> Users::updateUserCredentials(
	int user_id,
	string_view new_password
) {
	string query = "UPDATE USERS SET password = '" + string(new_password) + 
					   "' WHERE user_id = " + to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

expected<bool, string> Users::updateUserRoles(
	int user_id,
	string_view new_roles
) {
	string query = "UPDATE USERS SET roles = '" + string(new_roles) + 
					   "' WHERE user_id = " + to_string(user_id) + ";";

	auto result = database::DatabaseManager::getInstance().executeUpdate(query);

	if (!result) {
		return unexpected(result.error());
	}

	return true;
}

expected<bool, string> Users::deleteUser(int user_id, string roles) {
    auto& db = database::DatabaseManager::getInstance();
    string idStr = to_string(user_id);

	// 1. Set is_delete to 1 for the user in USERS table (soft delete)
	auto resSoftDelete = db.executeUpdate("UPDATE USERS SET is_deleted = 1 WHERE user_id = " + idStr + ";");
	if (!resSoftDelete) return unexpected(resSoftDelete.error());
	(void)resSoftDelete; // Mark as used to avoid unused variable warning

	if (roles.find("staff") != string::npos) {
		auto resStaffSoftDelete = db.executeUpdate("UPDATE STAFF SET is_deleted = 1 WHERE user_id = " + idStr + ";");
		if (!resStaffSoftDelete) return unexpected(resStaffSoftDelete.error());
	}
	else if (roles.find("customer") != string::npos) {
		auto resCustomerSoftDelete = db.executeUpdate("UPDATE CUSTOMERS SET is_deleted = 1 WHERE user_id = " + idStr + ";");
		if (!resCustomerSoftDelete) return unexpected(resCustomerSoftDelete.error());
	}

	auto checkBankAccoutnResult = db.executeQuery("SELECT acc_id FROM BANK_ACCOUNT WHERE user_id = " + idStr + " AND is_deleted = 0;");
	if (!checkBankAccoutnResult) return unexpected(checkBankAccoutnResult.error());

	sql::ResultSet* rsBank = checkBankAccoutnResult.value();
	bool hasBank = rsBank->next();
	delete rsBank;

	if (hasBank) {
		// 2. Set is_delete to 1 for the user's bank account (soft delete)
		auto resBankSoftDelete = db.executeUpdate("UPDATE BANK_ACCOUNT SET is_deleted = 1 WHERE user_id = " + idStr + ";");
		if (!resBankSoftDelete) return unexpected(resBankSoftDelete.error());
	}

    return true;
}

void Users::displayAllUsers() {
	auto usersResult = getAllUsers();

	if (!usersResult) {
		println("Error: {}", usersResult.error());
		return;
	}

	println("\n{:<10} {:<20} {:<20}", "User ID", "Username", "Roles");
	println("{:-<64}", "");

	for (const auto& user : usersResult.value()) {
		string roles;
		for (size_t i = 0; i < user.roles.size(); ++i) {
			roles += user.roles[i];
			if (i < user.roles.size() - 1) roles += ", ";
		}
		println("{:<10} {:<20} {:<20}", user.user_id, user.username, roles);
	}
	println("");
}

void Users::displayStaffManagementMenu() {
	println("\n╔═════════════════════════════════════╗");
	println("║      STAFF ACCOUNT MANAGEMENT       ║");
	println("╠═════════════════════════════════════╣");
	println("║  1. Create New Staff Account        ║");
	println("║  2. View All Users                  ║");
	println("║  3. Update User Roles               ║");
	println("║  4. Delete User Account             ║");
	println("║  5. Back to Main Menu                ║");
	println("╚═════════════════════════════════════╝");
	print("\nEnter your choice: ");
}

} // namespace identity::users
