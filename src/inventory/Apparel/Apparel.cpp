#include "inventory/Apparel/Apparel.h"
#include "DatabaseManager/DatabaseManager.h"
#include <format>
#include <iostream>

namespace inventory::apparel {

    std::expected<void, std::string> addApparelCatalog(const ApparelCatalog& catalog, const std::vector<ItemBatch>& batches) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string catalog_query = std::format(
            "INSERT INTO apparel_catalog (shop_id, name, description, category, colour, daily_rate) "
            "VALUES ({}, '{}', '{}', '{}', '{}', {})",
            catalog.shop_id, catalog.name, catalog.description, catalog.category, catalog.colour, catalog.daily_rate
        );

        auto catalog_result = db.executeUpdate(catalog_query);
        if (!catalog_result) return std::unexpected(catalog_result.error());

        auto id_res = db.executeQuery("SELECT LAST_INSERT_ID() AS last_id");
        if (!id_res) return std::unexpected("Failed to retrieve catalog ID");
        
        sql::ResultSet* res = id_res.value();
        int catalog_id = -1;
        if (res->next()) {
            catalog_id = res->getInt("last_id");
        }
        delete res;

        if (catalog_id == -1) return std::unexpected("Could not get new catalog ID.");

        for (const auto& batch : batches) {
            for (int i = 0; i < batch.quantity; ++i) {
                std::string item_query = std::format(
                    "INSERT INTO apparel_item (catalog_id, size, status, condition_status) "
                    "VALUES ({}, '{}', 'Available', '{}')",
                    catalog_id, batch.size, batch.condition
                );
                auto item_res = db.executeUpdate(item_query);
                if (!item_res) {
                    return std::unexpected("Error inserting physical item: " + item_res.error());
                }
            }
        }

        return {};
    }

    std::expected<int, std::string> getTotalApparelsCount(std::string_view searchTerm) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string query = "SELECT COUNT(*) AS total FROM apparel_catalog WHERE is_deleted = 0";
        if (!searchTerm.empty()) {
            query += std::format(" AND (name LIKE '%{}%' OR category LIKE '%{}%')", searchTerm, searchTerm);
        }
        
        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        sql::ResultSet* res = result.value();
        int count = 0;
        if (res->next()) {
            count = res->getInt("total");
        }
        delete res;
        return count;
    }

    std::expected<std::vector<CatalogDisplayItem>, std::string> getCatalogDisplay(int limit, int offset, std::string_view searchTerm) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string query = 
            "SELECT c.catalog_id, c.shop_id, c.name, c.category, c.daily_rate, "
            "(SELECT COUNT(*) FROM apparel_item i WHERE i.catalog_id = c.catalog_id AND i.status = 'Available' AND i.is_deleted = 0) AS available_stock "
            "FROM apparel_catalog c WHERE c.is_deleted = 0";
            
        if (!searchTerm.empty()) {
            query += std::format(" AND (c.name LIKE '%{}%' OR c.category LIKE '%{}%')", searchTerm, searchTerm);
        }
        
        query += std::format(" LIMIT {} OFFSET {}", limit, offset);
        
        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        std::vector<CatalogDisplayItem> items;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            CatalogDisplayItem item;
            item.catalog_id = res->getInt("catalog_id");
            item.shop_id = res->getInt("shop_id");
            item.name = res->getString("name");
            item.category = res->getString("category");
            item.daily_rate = res->getDouble("daily_rate");
            item.available_stock = res->getInt("available_stock");
            items.push_back(item);
        }
        delete res;
        return items;
    }

    std::expected<ApparelCatalog, std::string> getApparelById(int catalog_id) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "SELECT catalog_id, shop_id, name, description, category, colour, daily_rate "
            "FROM apparel_catalog WHERE catalog_id = {} AND is_deleted = 0", catalog_id
        );

        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        sql::ResultSet* res = result.value();
        if (res->next()) {
            ApparelCatalog catalog;
            catalog.catalog_id = res->getInt("catalog_id");
            catalog.shop_id = res->getInt("shop_id");
            catalog.name = res->getString("name");
            catalog.description = res->getString("description");
            catalog.category = res->getString("category");
            catalog.colour = res->getString("colour");
            catalog.daily_rate = res->getDouble("daily_rate");
            delete res;
            return catalog;
        }
        delete res;
        return std::unexpected("Catalog item not found.");
    }

    std::expected<std::vector<std::pair<std::string, int>>, std::string> getAvailableSizes(int catalog_id) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "SELECT size, COUNT(*) as qty FROM apparel_item "
            "WHERE catalog_id = {} AND status = 'Available' AND is_deleted = 0 "
            "GROUP BY size ORDER BY size", catalog_id
        );

        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        std::vector<std::pair<std::string, int>> sizes;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            sizes.push_back({res->getString("size"), res->getInt("qty")});
        }
        delete res;
        return sizes;
    }
}
