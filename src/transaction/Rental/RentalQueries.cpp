#include "transaction/Rental/Rental.h"
#include "transaction/Rental/RentalInternal.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <format>
#include <algorithm>
#include <vector>
#include <expected>

namespace db = ::database;

using namespace std;

namespace transaction::rental {

    expected<vector<RentalHistoryItem>, string> getCustomerRentalHistory(int user_id) {
        auto& db = db::DatabaseManager::getInstance();
        
        string query = format(
            "SELECT "
            "    r.rental_id, "
            "    r.unique_id, "
            "    r.rental_date, "
            "    r.expected_return_date, "
            "    COALESCE(DATE_FORMAT(rd.actual_return_date, '%Y-%m-%d'), 'Not Returned') AS actual_return_date, "
            "    COALESCE(c.name, 'Unknown') AS item_name, "
            "    COALESCE(i.size, 'N/A') AS size, "
            "    COALESCE(inv.base_fee, 0.00) AS base_fee, "
            "    COALESCE(inv.late_fee, 0.00) AS late_fee, "
            "    COALESCE(inv.payment_status, 'Pending') AS payment_status, "
            "    COALESCE(rd.return_condition, 'N/A') AS return_condition "
            "FROM rental r "
            "JOIN customers cust ON r.cust_id = cust.cust_id "
            "LEFT JOIN rental_details rd ON r.rental_id = rd.rental_id "
            "LEFT JOIN apparel_item i ON rd.item_id = i.item_id "
            "LEFT JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "LEFT JOIN invoices inv ON r.rental_id = inv.rental_id "
            "WHERE cust.user_id = {} AND r.is_deleted = 0 "
            "ORDER BY r.rental_date DESC",
            user_id
        );
        
        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());
        
        vector<RentalHistoryItem> history;
        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            RentalHistoryItem item;
            item.rental_id = rs->getInt("rental_id");
            item.unique_id = rs->getString("unique_id");
            
            string rDate = rs->getString("rental_date");
            item.rental_date = fromMySqlDate(rDate);
            
            string eDate = rs->getString("expected_return_date");
            item.expected_return_date = fromMySqlDate(eDate);
            
            string actDate = rs->getString("actual_return_date");
            item.actual_return_date = fromMySqlDate(actDate);

            item.item_name = rs->getString("item_name");
            item.size = rs->getString("size");
            item.base_fee = rs->getDouble("base_fee");
            item.late_fee = rs->getDouble("late_fee");
            item.payment_status = rs->getString("payment_status");
            item.return_condition = rs->getString("return_condition");
            
            history.push_back(item);
        }
        delete rs;
        return history;
    }

    expected<BookingStats, string> getCustomerBookingStats(int user_id) {
        auto& db = db::DatabaseManager::getInstance();
        BookingStats stats;
        stats.return_behaviour = {0, 0, 0, 0};

        // 1. Query Category Popularity
        string catQuery = format(
            "SELECT c.category, COUNT(*) AS count "
            "FROM rental r "
            "JOIN rental_details rd ON r.rental_id = rd.rental_id "
            "JOIN apparel_item i ON rd.item_id = i.item_id "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "JOIN customers cust ON r.cust_id = cust.cust_id "
            "WHERE cust.user_id = {} AND r.is_deleted = 0 "
            "GROUP BY c.category "
            "ORDER BY count DESC",
            user_id
        );
        auto catRes = db.executeQuery(catQuery);
        if (catRes) {
            sql::ResultSet* rs = catRes.value();
            while (rs->next()) {
                stats.categories.push_back({
                    rs->getString("category"),
                    rs->getInt("count")
                });
            }
            delete rs;
        }

        // 2. Query Return Behaviour ratios
        string retQuery = format(
            "SELECT "
            "  COALESCE(SUM(CASE WHEN rd.actual_return_date IS NOT NULL AND rd.actual_return_date <= r.expected_return_date THEN 1 ELSE 0 END), 0) AS on_time_count, "
            "  COALESCE(SUM(CASE WHEN rd.actual_return_date IS NOT NULL AND rd.actual_return_date > r.expected_return_date THEN 1 ELSE 0 END), 0) AS late_count, "
            "  COALESCE(SUM(CASE WHEN rd.actual_return_date IS NULL AND CURRENT_DATE() > r.expected_return_date THEN 1 ELSE 0 END), 0) AS overdue_active_count, "
            "  COALESCE(SUM(CASE WHEN rd.actual_return_date IS NULL AND CURRENT_DATE() <= r.expected_return_date THEN 1 ELSE 0 END), 0) AS on_time_active_count "
            "FROM rental r "
            "JOIN rental_details rd ON r.rental_id = rd.rental_id "
            "JOIN customers cust ON r.cust_id = cust.cust_id "
            "WHERE cust.user_id = {} AND r.is_deleted = 0",
            user_id
        );
        auto retRes = db.executeQuery(retQuery);
        if (retRes) {
            sql::ResultSet* rs = retRes.value();
            if (rs->next()) {
                stats.return_behaviour.on_time = rs->getInt("on_time_count");
                stats.return_behaviour.late = rs->getInt("late_count");
                stats.return_behaviour.active_on_time = rs->getInt("on_time_active_count");
                stats.return_behaviour.active_overdue = rs->getInt("overdue_active_count");
            }
            delete rs;
        }

        // 3. Query Booking Frequency trends over 6 months
        string trendQuery = format(
            "SELECT DATE_FORMAT(r.rental_date, '%b %Y') AS month_name, COUNT(*) AS count "
            "FROM rental r "
            "JOIN customers cust ON r.cust_id = cust.cust_id "
            "WHERE cust.user_id = {} AND r.is_deleted = 0 "
            "GROUP BY month_name, DATE_FORMAT(r.rental_date, '%Y-%m') "
            "ORDER BY DATE_FORMAT(r.rental_date, '%Y-%m') DESC "
            "LIMIT 6",
            user_id
        );
        auto trendRes = db.executeQuery(trendQuery);
        if (trendRes) {
            sql::ResultSet* rs = trendRes.value();
            while (rs->next()) {
                stats.monthly_trends.push_back({
                    rs->getString("month_name"),
                    rs->getInt("count")
                });
            }
            // Reverse so months flow chronologically left-to-right (past to present)
            reverse(stats.monthly_trends.begin(), stats.monthly_trends.end());
            delete rs;
        }

        return stats;
    }

    expected<vector<ActiveRentalItem>, string> getActiveRentals(const string& searchTerm) {
        auto& db = db::DatabaseManager::getInstance();
        
        string query = 
            "SELECT r.rental_id, r.unique_id, cust.fullname AS customer_name, c.name AS item_name, i.size, "
            "       r.rental_date, r.expected_return_date, c.daily_rate, COALESCE(inv.payment_status, 'Paid') AS payment_status, "
            "       CASE WHEN CURRENT_DATE() > r.expected_return_date THEN 1 ELSE 0 END AS is_overdue "
            "FROM rental r "
            "JOIN customers cust ON r.cust_id = cust.cust_id "
            "JOIN rental_details rd ON r.rental_id = rd.rental_id "
            "JOIN apparel_item i ON rd.item_id = i.item_id "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "LEFT JOIN invoices inv ON r.rental_id = inv.rental_id "
            "WHERE rd.actual_return_date IS NULL AND r.is_deleted = 0";

        if (!searchTerm.empty()) {
            query += format(" AND (cust.fullname LIKE '%{}%' OR c.name LIKE '%{}%')", searchTerm, searchTerm);
        }

        query += " ORDER BY is_overdue DESC, r.expected_return_date ASC";

        auto result = db.executeQuery(query);
        if (!result) return unexpected(result.error());

        vector<ActiveRentalItem> items;
        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            ActiveRentalItem item;
            item.rental_id = rs->getInt("rental_id");
            item.unique_id = rs->getString("unique_id");
            item.customer_name = rs->getString("customer_name");
            item.item_name = rs->getString("item_name");
            item.size = rs->getString("size");
            
            string rDate = rs->getString("rental_date");
            item.rental_date = fromMySqlDate(rDate);
            
            string eDate = rs->getString("expected_return_date");
            item.expected_return_date = fromMySqlDate(eDate);

            item.daily_rate = rs->getDouble("daily_rate");
            item.payment_status = rs->getString("payment_status");
            item.is_overdue = (rs->getInt("is_overdue") == 1);
            
            items.push_back(item);
        }
        delete rs;
        return items;
    }

}