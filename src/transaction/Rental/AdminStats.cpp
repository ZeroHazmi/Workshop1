#include "transaction/Rental/AdminStats.h"
#include "DatabaseManager/DatabaseManager.h"
#include <format>
#include <algorithm>
#include <iostream>

using namespace std;

namespace transaction::rental::stats {

    // Helper to format a vector of shop IDs into a dynamic MySQL IN list, e.g. "(1, 3)"
    string buildShopIdsList(const vector<int>& shopIds) {
        if (shopIds.empty()) return "";
        string list = "(";
        for (size_t i = 0; i < shopIds.size(); ++i) {
            list += to_string(shopIds[i]);
            if (i + 1 < shopIds.size()) {
                list += ", ";
            }
        }
        list += ")";
        return list;
    }

    // Helper to construct dynamic date range filters
    string buildDateFilter(const DateRange& dateRange) {
        string filter = "";
        if (!dateRange.startDate.empty()) {
            filter += "AND r.rental_date >= '" + dateRange.startDate + " 00:00:00' ";
        }
        if (!dateRange.endDate.empty()) {
            filter += "AND r.rental_date <= '" + dateRange.endDate + " 23:59:59' ";
        }
        return filter;
    }

    expected<vector<RevenueTrendPoint>, string> getRevenueTrends(
        const vector<int>& shopIds, 
        const DateRange& dateRange
    ) {
        auto& db = database::DatabaseManager::getInstance();
        vector<RevenueTrendPoint> trends;

        string shopFilter = "";
        if (!shopIds.empty()) {
            shopFilter = "AND r.shop_id IN " + buildShopIdsList(shopIds) + " ";
        }

        string dateFilter = buildDateFilter(dateRange);

        // Dynamically adjust LIMIT based on whether custom date filtering is active
        string limitClause = "LIMIT 6";
        if (!dateRange.startDate.empty() || !dateRange.endDate.empty()) {
            limitClause = ""; // Pull all matching months in specified range
        }

        // Query aggregating last 6 months (or custom date range) of base + late fee revenue
        string query = 
            "SELECT DATE_FORMAT(r.rental_date, '%b %Y') AS month_name, "
            "       SUM(COALESCE(inv.base_fee + inv.late_fee, 0.00)) AS revenue, "
            "       DATE_FORMAT(r.rental_date, '%Y-%m') AS sort_month "
            "FROM rental r "
            "JOIN invoices inv ON r.rental_id = inv.rental_id "
            "WHERE r.is_deleted = 0 " + shopFilter + dateFilter +
            "GROUP BY month_name, sort_month "
            "ORDER BY sort_month DESC " + limitClause + ";";

        auto result = db.executeQuery(query);
        if (!result) {
            return unexpected("Failed to query revenue trends: " + result.error());
        }

        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            trends.push_back({
                rs->getString("month_name"),
                static_cast<double>(rs->getDouble("revenue"))
            });
        }
        delete rs;

        // Reverse so months flow chronologically (past to present)
        reverse(trends.begin(), trends.end());
        return trends;
    }

    expected<vector<CostumePopularityPoint>, string> getCostumePopularity(
        const vector<int>& shopIds, 
        const DateRange& dateRange
    ) {
        auto& db = database::DatabaseManager::getInstance();
        vector<CostumePopularityPoint> points;

        string shopFilter = "";
        if (!shopIds.empty()) {
            shopFilter = "AND r.shop_id IN " + buildShopIdsList(shopIds) + " ";
        }

        string dateFilter = buildDateFilter(dateRange);

        string query = 
            "SELECT c.name AS catalog_name, COUNT(*) AS rental_count "
            "FROM rental r "
            "JOIN rental_details rd ON r.rental_id = rd.rental_id "
            "JOIN apparel_item i ON rd.item_id = i.item_id "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "WHERE r.is_deleted = 0 " + shopFilter + dateFilter +
            "GROUP BY c.catalog_id, c.name "
            "ORDER BY rental_count DESC "
            "LIMIT 5;";

        auto result = db.executeQuery(query);
        if (!result) {
            return unexpected("Failed to query costume popularity: " + result.error());
        }

        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            points.push_back({
                rs->getString("catalog_name"),
                rs->getInt("rental_count")
            });
        }
        delete rs;
        return points;
    }

    expected<vector<BranchPerformancePoint>, string> getBranchPerformance(
        const vector<int>& shopIds, 
        const DateRange& dateRange
    ) {
        auto& db = database::DatabaseManager::getInstance();
        vector<BranchPerformancePoint> points;

        string shopFilter = "";
        if (!shopIds.empty()) {
            shopFilter = "AND r.shop_id IN " + buildShopIdsList(shopIds) + " ";
        }

        string dateFilter = buildDateFilter(dateRange);

        string primaryQuery = 
            "SELECT COALESCE(s.shop_name, CONCAT('Branch #', r.shop_id)) AS shop_name, "
            "       SUM(COALESCE(inv.base_fee + inv.late_fee, 0.00)) AS revenue "
            "FROM rental r "
            "JOIN invoices inv ON r.rental_id = inv.rental_id "
            "LEFT JOIN shops s ON r.shop_id = s.shop_id "
            "WHERE r.is_deleted = 0 " + shopFilter + dateFilter +
            "GROUP BY r.shop_id, s.shop_name "
            "ORDER BY revenue DESC;";

        auto result = db.executeQuery(primaryQuery);
        if (!result) {
            // Graceful fallback query
            string fallbackQuery = 
                "SELECT CONCAT('Branch #', r.shop_id) AS shop_name, "
                "       SUM(COALESCE(inv.base_fee + inv.late_fee, 0.00)) AS revenue "
                "FROM rental r "
                "JOIN invoices inv ON r.rental_id = inv.rental_id "
                "WHERE r.is_deleted = 0 " + shopFilter + dateFilter +
                "GROUP BY r.shop_id "
                "ORDER BY revenue DESC;";
                
            result = db.executeQuery(fallbackQuery);
        }

        if (!result) {
            return unexpected("Failed to query branch performance: " + result.error());
        }

        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            points.push_back({
                rs->getString("shop_name"),
                static_cast<double>(rs->getDouble("revenue"))
            });
        }
        delete rs;
        return points;
    }

    expected<vector<InventoryConditionPoint>, string> getInventoryConditionAudit(
        const vector<int>& shopIds
    ) {
        auto& db = database::DatabaseManager::getInstance();
        vector<InventoryConditionPoint> points;

        string query;
        if (!shopIds.empty()) {
            string shopFilter = "AND c.shop_id IN " + buildShopIdsList(shopIds) + " ";
            query = 
                "SELECT i.condition_status AS condition_status, COUNT(*) AS count "
                "FROM apparel_item i "
                "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
                "WHERE i.is_deleted = 0 AND c.is_deleted = 0 " + shopFilter +
                "GROUP BY i.condition_status "
                "ORDER BY count DESC;";
        } else {
            query = 
                "SELECT condition_status AS condition_status, COUNT(*) AS count "
                "FROM apparel_item "
                "WHERE is_deleted = 0 "
                "GROUP BY condition_status "
                "ORDER BY count DESC;";
        }

        auto result = db.executeQuery(query);
        if (!result) {
            return unexpected("Failed to query inventory condition: " + result.error());
        }

        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            points.push_back({
                rs->getString("condition_status"),
                rs->getInt("count")
            });
        }
        delete rs;
        return points;
    }

} // namespace transaction::rental::stats
