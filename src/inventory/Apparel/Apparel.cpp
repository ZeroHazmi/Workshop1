#include "inventory/Apparel/Apparel.h"
#include "DatabaseManager/DatabaseManager.h"
#include <format>
#include <iostream>

namespace db = ::database;

using namespace std;

namespace inventory::apparel {

    expected<void, string> addApparelCatalog(const ApparelCatalog& catalog, const vector<ItemBatch>& batches) {
        auto& db = db::DatabaseManager::getInstance();
        
        string catalogUniqueId = db::DatabaseManager::generateUniqueId("CAT");

        string catalog_query = format(
            "INSERT INTO apparel_catalog (shop_id, name, description, category, colour, daily_rate, unique_id) "
            "VALUES ({}, '{}', '{}', '{}', '{}', {}, '{}')",
            catalog.shop_id, catalog.name, catalog.description, catalog.category, catalog.colour, catalog.daily_rate, catalogUniqueId
        );

        auto catalog_result = db.executeUpdate(catalog_query);
        if (!catalog_result) return unexpected(catalog_result.error());

        auto id_res = db.executeQuery("SELECT LAST_INSERT_ID() AS last_id");
        if (!id_res) return unexpected("Failed to retrieve catalog ID");
        
        sql::ResultSet* res = id_res.value();
        int catalog_id = -1;
        if (res->next()) {
            catalog_id = res->getInt("last_id");
        }
        delete res;

        if (catalog_id == -1) return unexpected("Could not get new catalog ID.");

        for (const auto& batch : batches) {
            for (int i = 0; i < batch.quantity; ++i) {
                string itemUniqueId = db::DatabaseManager::generateUniqueId("ITM");
                string item_query = format(
                    "INSERT INTO apparel_item (catalog_id, size, status, condition_status, unique_id) "
                    "VALUES ({}, '{}', 'Available', '{}', '{}')",
                    catalog_id, batch.size, batch.condition, itemUniqueId
                );
                auto item_res = db.executeUpdate(item_query);
                if (!item_res) {
                    return unexpected("Error inserting physical item: " + item_res.error());
                }
            }
        }

        return {};
    }

    expected<int, string> getTotalApparelsCount(string_view searchTerm) {
        auto& db = db::DatabaseManager::getInstance();
        
        string query = "SELECT COUNT(*) AS total FROM apparel_catalog WHERE is_deleted = 0";
        if (!searchTerm.empty()) {
            query += format(" AND (name LIKE '%{}%' OR category LIKE '%{}%')", searchTerm, searchTerm);
        }
        
        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        sql::ResultSet* res = result.value();
        int count = 0;
        if (res->next()) {
            count = res->getInt("total");
        }
        delete res;
        return count;
    }

    expected<vector<CatalogDisplayItem>, string> getCatalogDisplay(int limit, int offset, string_view searchTerm) {
        auto& db = db::DatabaseManager::getInstance();
        
        string query = 
            "SELECT c.catalog_id, c.unique_id, c.shop_id, c.name, c.category, c.daily_rate, "
            "(SELECT COUNT(*) FROM apparel_item i WHERE i.catalog_id = c.catalog_id AND i.status = 'Available' AND i.is_deleted = 0) AS available_stock "
            "FROM apparel_catalog c WHERE c.is_deleted = 0";
            
        if (!searchTerm.empty()) {
            query += format(" AND (c.name LIKE '%{}%' OR c.category LIKE '%{}%')", searchTerm, searchTerm);
        }
        
        query += format(" LIMIT {} OFFSET {}", limit, offset);
        
        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        vector<CatalogDisplayItem> items;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            CatalogDisplayItem item;
            item.catalog_id = res->getInt("catalog_id");
            item.unique_id = res->getString("unique_id");
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

    expected<ApparelCatalog, string> getApparelById(int catalog_id) {
        auto& db = db::DatabaseManager::getInstance();
        string query = format(
            "SELECT catalog_id, unique_id, shop_id, name, description, category, colour, daily_rate "
            "FROM apparel_catalog WHERE catalog_id = {} AND is_deleted = 0", catalog_id
        );

        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        sql::ResultSet* res = result.value();
        if (res->next()) {
            ApparelCatalog catalog;
            catalog.catalog_id = res->getInt("catalog_id");
            catalog.unique_id = res->getString("unique_id");
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
        return unexpected("Catalog item not found.");
    }

    expected<ApparelCatalog, string> getApparelByUniqueId(string_view unique_id) {
        auto& db = db::DatabaseManager::getInstance();
        string query = format(
            "SELECT catalog_id, unique_id, shop_id, name, description, category, colour, daily_rate "
            "FROM apparel_catalog WHERE (unique_id = '{}' OR catalog_id = '{}') AND is_deleted = 0",
            string(unique_id), string(unique_id)
        );

        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        sql::ResultSet* res = result.value();
        if (res->next()) {
            ApparelCatalog catalog;
            catalog.catalog_id = res->getInt("catalog_id");
            catalog.unique_id = res->getString("unique_id");
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
        return unexpected("Catalog item not found.");
    }

    expected<vector<pair<string, int>>, string> getAvailableSizes(int catalog_id) {
        auto& db = db::DatabaseManager::getInstance();
        string query = format(
            "SELECT size, COUNT(*) as qty FROM apparel_item "
            "WHERE catalog_id = {} AND status = 'Available' AND is_deleted = 0 "
            "GROUP BY size ORDER BY size", catalog_id
        );

        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        vector<pair<string, int>> sizes;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            sizes.push_back({res->getString("size"), res->getInt("qty")});
        }
        delete res;
        return sizes;
    }

    expected<vector<ApparelItem>, string> getItemsByStatus(string_view status) {
        auto& db = db::DatabaseManager::getInstance();
        string query = format(
            "SELECT item_id, unique_id, catalog_id, size, status, condition_status FROM apparel_item "
            "WHERE status = '{}' AND is_deleted = 0",
            status
        );

        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        vector<ApparelItem> items;
        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            ApparelItem item;
            item.item_id = rs->getInt("item_id");
            item.unique_id = rs->getString("unique_id");
            item.catalog_id = rs->getInt("catalog_id");
            item.size = rs->getString("size");
            item.status = rs->getString("status");
            item.condition_status = rs->getString("condition_status");
            items.push_back(item);
        }
        delete rs;
        return items;
    }

    expected<void, string> updateItemCondition(int item_id, string_view condition) {
        auto& db = db::DatabaseManager::getInstance();
        string query = format(
            "UPDATE apparel_item SET condition_status = '{}' WHERE item_id = {} AND is_deleted = 0",
            condition, item_id
        );
        auto result = db.executeUpdate(query);
        if (!result) return unexpected(result.error());
        return {};
    }

    expected<void, string> updateItemStatus(int item_id, string_view status) {
        auto& db = db::DatabaseManager::getInstance();
        string query = format(
            "UPDATE apparel_item SET status = '{}' WHERE item_id = {} AND is_deleted = 0",
            status, item_id
        );
        auto result = db.executeUpdate(query);
        if (!result) return unexpected(result.error());
        return {};
    }
}