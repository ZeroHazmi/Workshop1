#pragma once

#include <string>
#include <vector>
#include <expected>

struct ApparelCatalog {
	int catalog_id; // Primary Key
	int shop_id;    // Foreign Key -> Shops.shop_id
	std::string description;
	std::string category;
	double daily_rate;
	std::string size;
	std::string colour;
};

struct ApparelItem {
	int item_id;     // Primary Key
	int catalog_id;  // Foreign Key -> ApparelCatalog
	std::string status; // e.g., Available/Rented/Laundry/Maintenance
	std::string condition_status; // e.g., Excellent, Good, Damaged
};

// Joined struct for display purposes
struct CatalogDisplayItem {
    int catalog_id;
    int shop_id;
    std::string description;
    std::string category;
    double daily_rate;
    int available_stock; // Calculated from COUNT(item_id) where status='Available'
};

namespace inventory::apparel {
    // Add New Apparel Catalog (and generate N physical items)
    std::expected<void, std::string> addApparelCatalog(const ApparelCatalog& catalog, int initial_stock, std::string_view initial_condition);

    // Retrieve all apparel for catalog display (with optional search)
    std::expected<std::vector<CatalogDisplayItem>, std::string> getCatalogDisplay(std::string_view searchTerm = "");

    // Retrieve items needing laundry/maintenance
    std::expected<std::vector<ApparelItem>, std::string> getItemsByStatus(std::string_view status);

    // Update specific item status/condition
    std::expected<void, std::string> updateItemCondition(int item_id, std::string_view condition);
    std::expected<void, std::string> updateItemStatus(int item_id, std::string_view status);
}
