
#pragma once

#include <string>
#include <vector>
#include <expected>
#include <string_view>

namespace identify {
	struct User {
		int user_id; // Primary Key
		std::string unique_id;
		std::string username;
		std::string password;
		std::vector<std::string> roles;
	};

	struct StaffRecord {
		int staff_id; // Primary Key
		std::string unique_id;
		int user_id;  // Foreign Key -> Users.user_id
		int shop_id;  // Foreign Key -> Shops.shop_id
		std::string staff_name;
		std::string position;
		std::string phone_no;
	};

	struct Customer {
		int cust_id; // Primary Key
		std::string unique_id;
		int user_id; // Foreign Key -> Users.user_id
		std::string fullname;
		std::string phone_no;
		std::string email;
	};
}

namespace identity::users {

class Users {
public:
	Users() = default;
	~Users() = default;

	// READ: Get all users
	static std::expected<std::vector<identify::User>, std::string> getAllUsers();

	// READ: Get user by ID
	static std::expected<identify::User, std::string> getUserById(int user_id);

	// READ: Get user by username
	static std::expected<identify::User, std::string> getUserByUsername(std::string_view username);

	// READ: Get user by Unique ID
	static std::expected<identify::User, std::string> getUserByUniqueId(std::string_view unique_id);

	// CREATE: Add new staff user (Admin operation)
	static std::expected<int, std::string> createStaffUser(
		std::string_view username,
		std::string_view password,
		std::string_view staff_name,
		std::string_view position,
		std::string_view phone_no,
		int shop_id
	);

	// UPDATE: Update user credentials
	static std::expected<bool, std::string> updateUserCredentials(
		int user_id,
		std::string_view new_password
	);

	// UPDATE: Update user roles
	static std::expected<bool, std::string> updateUserRoles(
		int user_id,
		std::string_view new_roles
	);

	// DELETE: Remove user (Admin operation)
	static std::expected<bool, std::string> deleteUser(int user_id, std::string roles);
	
	// Helper: Display all users in table format
	static void displayAllUsers();

	// Helper: Display staff management menu
	static void displayStaffManagementMenu();
};

} // namespace identity::users
