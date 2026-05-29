#pragma once
#pragma once

#include <string>
#include <vector>
#include <expected>
#include <string_view>
#include <utility>

namespace inventory::apparel {

    struct ApparelCatalog {
        int catalog_id;
        int shop_id;
        std::string name;
        std::string description;
        std::string category;
        std::string colour;
        double daily_rate;
    };

    struct ApparelItem {
        int item_id;
        int catalog_id;
        std::string size;
        std::string status;
        std::string condition_status;
    };

    struct CatalogDisplayItem {
        int catalog_id;
        int shop_id;
        std::string name;
        std::string category;
        double daily_rate;
        int available_stock;
    };

    struct ItemBatch {
        std::string size;
        int quantity;
        std::string condition;
    };

    // Add New Apparel Catalog (and generate physical items from batches)
    std::expected<void, std::string> addApparelCatalog(const ApparelCatalog& catalog, const std::vector<ItemBatch>& batches);

    // Get the total number of distinct catalogs for pagination
    std::expected<int, std::string> getTotalApparelsCount(std::string_view searchTerm = "");

    // Retrieve apparel for catalog display with pagination and optional search
    std::expected<std::vector<CatalogDisplayItem>, std::string> getCatalogDisplay(int limit, int offset, std::string_view searchTerm = "");

    // Get a specific catalog item by its ID
    std::expected<ApparelCatalog, std::string> getApparelById(int catalog_id);

    // Get available sizes and quantities for a given catalog
    std::expected<std::vector<std::pair<std::string, int>>, std::string> getAvailableSizes(int catalog_id);

}
