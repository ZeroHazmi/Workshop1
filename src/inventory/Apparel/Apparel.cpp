#include "inventory/Apparel/Apparel.h"
#include "DatabaseManager/DatabaseManager.h"
#include <format>
#include <iostream>

namespace inventory::apparel {

    std::expected<void, std::string> addApparelCatalog(const ApparelCatalog& catalog, int initial_stock, std::string_view initial_condition) {
        auto& db = database::DatabaseManager::getInstance();
        
        // 1. Insert into APPAREL_CATALOG
        std::string catalog_query = std::format(
            "INSERT INTO APPAREL_CATALOG (shop_id, description, category, daily_rate, size, colour) "
            "VALUES ({}, '{}', '{}', {}, '{}', '{}')",
            catalog.shop_id, catalog.description, catalog.category, catalog.daily_rate, catalog.size, catalog.colour
        );

        auto catalog_result = db.executeUpdate(catalog_query);
        if (!catalog_result) {
            return std::unexpected(catalog_result.error());
        }

        // We need the ID of the inserted catalog to create items
        // MySQL LAST_INSERT_ID() can be used. Let's run a select.
        auto id_res = db.executeQuery("SELECT LAST_INSERT_ID() AS last_id");
        if (!id_res) return std::unexpected("Failed to retrieve catalog ID");
        
        sql::ResultSet* res = id_res.value();
        int catalog_id = -1;
        if (res->next()) {
            catalog_id = res->getInt("last_id");
        }
        delete res;

        if (catalog_id == -1) return std::unexpected("Could not get new catalog ID.");

        // 2. Insert N items into APPAREL_ITEM
        for (int i = 0; i < initial_stock; ++i) {
            std::string item_query = std::format(
                "INSERT INTO APPAREL_ITEM (catalog_id, status, condition_status) "
                "VALUES ({}, 'Available', '{}')",
                catalog_id, std::string(initial_condition)
            );
            auto item_res = db.executeUpdate(item_query);
            if (!item_res) {
                // If one fails, we return error, though previous ones might be inserted (no transaction management yet)
                return std::unexpected("Error inserting physical item: " + item_res.error());
            }
        }

        return {};
    }

    std::expected<std::vector<CatalogDisplayItem>, std::string> getCatalogDisplay(std::string_view searchTerm) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string query = 
            "SELECT c.catalog_id, c.shop_id, c.description, c.category, c.daily_rate, "
            "(SELECT COUNT(*) FROM apparel_item i WHERE i.catalog_id = c.catalog_id AND i.status = 'Available') AS available_stock "
            "FROM apparel_catalog c";
            
        if (!searchTerm.empty()) {
            query += std::format(" WHERE c.description LIKE '%{}%' OR c.category LIKE '%{}%'", searchTerm, searchTerm);
        }
        
        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        std::vector<CatalogDisplayItem> items;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            CatalogDisplayItem item;
            item.catalog_id = res->getInt("catalog_id");
            item.shop_id = res->getInt("shop_id");
            item.description = res->getString("description");
            item.category = res->getString("category");
            item.daily_rate = res->getDouble("daily_rate");
            item.available_stock = res->getInt("available_stock");
            items.push_back(item);
        }
        delete res;
        return items;
    }

    std::expected<std::vector<ApparelItem>, std::string> getItemsByStatus(std::string_view status) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "SELECT item_id, catalog_id, status, condition_status FROM APPAREL_ITEM WHERE status = '{}'",
            status
        );

        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        std::vector<ApparelItem> items;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            ApparelItem item;
            item.item_id = res->getInt("item_id");
            item.catalog_id = res->getInt("catalog_id");
            item.status = res->getString("status");
            item.condition_status = res->getString("condition_status");
            items.push_back(item);
        }
        delete res;
        return items;
    }

    std::expected<void, std::string> updateItemCondition(int item_id, std::string_view condition) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "UPDATE APPAREL_ITEM SET condition_status = '{}' WHERE item_id = {}",
            condition, item_id
        );
        auto result = db.executeUpdate(query);
        if (!result) return std::unexpected(result.error());
        return {};
    }

    std::expected<void, std::string> updateItemStatus(int item_id, std::string_view status) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "UPDATE APPAREL_ITEM SET status = '{}' WHERE item_id = {}",
            status, item_id
        );
        auto result = db.executeUpdate(query);
        if (!result) return std::unexpected(result.error());
        return {};
    }
}
