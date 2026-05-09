#pragma once

#include <string>
#include <vector>
#include <expected>

struct ApparelItem {
	int apparel_id; // Primary Key
	int shop_id;    // Foreign Key -> Shops.shop_id
	std::string description;
	std::string category;
	double daily_rate;
	std::string condition_status;
	std::string status; // e.g., available/reserved/rented
	std::string size;
	std::string colour;
	int total_stock;
	int available_stock;
};

namespace inventory::apparel {
    // Add New Apparel
    std::expected<void, std::string> addApparel(const ApparelItem& item);

    // Retrieve all apparel
    std::expected<std::vector<ApparelItem>, std::string> getAllApparel();

    // Retrieve apparel by ID
    std::expected<ApparelItem, std::string> getApparelById(int id);
}
