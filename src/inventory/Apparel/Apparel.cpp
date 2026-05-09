#include "inventory/Apparel/Apparel.h"
#include "DatabaseManager/DatabaseManager.h"
#include <format>
#include <iostream>

namespace inventory::apparel {

    std::expected<void, std::string> addApparel(const ApparelItem& item) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string query = std::format(
            "INSERT INTO APPAREL (shop_id, description, category, daily_rate, condition_status, status, size, colour, total_stock, available_stock) "
            "VALUES ({}, '{}', '{}', {}, '{}', '{}', '{}', '{}', {}, {})",
            item.shop_id, item.description, item.category, item.daily_rate, item.condition_status, item.status, item.size, item.colour, item.total_stock, item.available_stock
        );

        auto result = db.executeUpdate(query);
        if (!result) {
            return std::unexpected(result.error());
        }
        return {};
    }

    std::expected<std::vector<ApparelItem>, std::string> getAllApparel() {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = "SELECT apparel_id, shop_id, description, category, daily_rate, condition_status, status, size, colour, total_stock, available_stock FROM APPAREL";
        
        auto result = db.executeQuery(query);
        if (!result) {
            return std::unexpected(result.error());
        }

        std::vector<ApparelItem> items;
        sql::ResultSet* res = result.value();
        while (res->next()) {
            ApparelItem item;
            item.apparel_id = res->getInt("apparel_id");
            item.shop_id = res->getInt("shop_id");
            item.description = res->getString("description");
            item.category = res->getString("category");
            item.daily_rate = res->getDouble("daily_rate");
            item.condition_status = res->getString("condition_status");
            item.status = res->getString("status");
            item.size = res->getString("size");
            item.colour = res->getString("colour");
            item.total_stock = res->getInt("total_stock");
            item.available_stock = res->getInt("available_stock");
            items.push_back(item);
        }
        delete res;
        return items;
    }

    std::expected<ApparelItem, std::string> getApparelById(int id) {
        auto& db = database::DatabaseManager::getInstance();
        std::string query = std::format(
            "SELECT apparel_id, shop_id, description, category, daily_rate, condition_status, status, size, colour, total_stock, available_stock FROM APPAREL WHERE apparel_id = {}", id
        );

        auto result = db.executeQuery(query);
        if (!result) {
            return std::unexpected(result.error());
        }

        sql::ResultSet* res = result.value();
        if (res->next()) {
            ApparelItem item;
            item.apparel_id = res->getInt("apparel_id");
            item.shop_id = res->getInt("shop_id");
            item.description = res->getString("description");
            item.category = res->getString("category");
            item.daily_rate = res->getDouble("daily_rate");
            item.condition_status = res->getString("condition_status");
            item.status = res->getString("status");
            item.size = res->getString("size");
            item.colour = res->getString("colour");
            item.total_stock = res->getInt("total_stock");
            item.available_stock = res->getInt("available_stock");
            delete res;
            return item;
        }
        delete res;
        return std::unexpected("Apparel not found.");
    }

}
