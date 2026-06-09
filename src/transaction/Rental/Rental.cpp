#include "transaction/Rental/Rental.h"
#include "DatabaseManager/DatabaseManager.h"
#include "tool/DateHelper.h"
#include <format>
#include <iostream>
#include <algorithm>
#include <vector>
#include <expected>

namespace transaction::rental {

    std::string toMySqlDate(const std::string &date) {
        if (date.length() != 10) return date;
        return date.substr(6, 4) + "-" + date.substr(3, 2) + "-" + date.substr(0, 2);
    }

    std::string fromMySqlDate(const std::string &date) {
        if (date.length() != 10) return date;
        return date.substr(8, 2) + "/" + date.substr(5, 2) + "/" + date.substr(0, 4);
    }

    std::expected<std::string, std::string> createRental(
        int user_id, int catalog_id, const std::string &size,
        const std::string &start_date, const std::string &expected_return_date,
        double daily_rate, int total_days) {
      auto &db = database::DatabaseManager::getInstance();

      // 1. Get cust_id from user_id
      std::string custQuery = std::format(
          "SELECT cust_id FROM customers WHERE user_id = {}", user_id);
      auto custRes = db.executeQuery(custQuery);
      if (!custRes)
        return std::unexpected("Database error finding customer: " +
                               custRes.error());

      sql::ResultSet *crs = custRes.value();
      if (!crs->next()) {
        delete crs;
        return std::unexpected(
            "Customer profile not found. Please complete your profile first.");
      }
      int cust_id = crs->getInt("cust_id");
      delete crs;

      // 2. Find an available item_id and get shop_id
      std::string itemQuery =
          std::format("SELECT i.item_id, c.shop_id FROM apparel_item i "
                      "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
                      "WHERE i.catalog_id = {} AND i.size = '{}' AND i.status "
                      "= 'Available' AND i.is_deleted = 0 "
                      "LIMIT 1",
                      catalog_id, size);
      auto itemRes = db.executeQuery(itemQuery);
      if (!itemRes)
        return std::unexpected("Database error finding item: " +
                               itemRes.error());

      sql::ResultSet *irs = itemRes.value();
      if (!irs->next()) {
        delete irs;
        return std::unexpected("Sorry, that size is no longer available.");
      }
      int item_id = irs->getInt("item_id");
      int shop_id = irs->getInt("shop_id");
      delete irs;

      // 3. Query Bank Account & Validate Balance
      std::string bankQuery = std::format(
          "SELECT acc_id, balance FROM BANK_ACCOUNT WHERE user_id = {} AND is_deleted = 0 LIMIT 1",
          user_id);
      auto bankRes = db.executeQuery(bankQuery);
      if (!bankRes) {
          return std::unexpected("Database error finding bank account: " + bankRes.error());
      }
      sql::ResultSet* brs = bankRes.value();
      if (!brs->next()) {
          delete brs;
          return std::unexpected("No linked bank account found. Please link a bank account and deposit funds first.");
      }
      int acc_id = brs->getInt("acc_id");
      double active_balance = brs->getDouble("balance");
      delete brs;

      double total_fee = daily_rate * total_days;
      // Hybrid Deposit: Flat hold of RM 50.00 or 50% of the total base fee, whichever is higher
      double deposit = std::max(50.00, total_fee * 0.50);
      double total_due = total_fee + deposit;

      if (active_balance < total_due) {
          return std::unexpected(std::format(
              "Insufficient funds. Total initial due is RM {:.2f} (Base: RM {:.2f} + Deposit: RM {:.2f}), but your account balance is RM {:.2f}.",
              total_due, total_fee, deposit, active_balance));
      }

      // Deduct the initial charge (Base fee + Security Deposit hold)
      std::string deductQuery = std::format(
          "UPDATE BANK_ACCOUNT SET balance = balance - {:.2f} WHERE acc_id = {}",
          total_due, acc_id);
      auto deductRes = db.executeUpdate(deductQuery);
      if (!deductRes) {
          return std::unexpected("Failed to process transaction charge: " + deductRes.error());
      }

      // 4. Insert into rental
      std::string sqlStart = toMySqlDate(start_date);
      std::string sqlEnd = toMySqlDate(expected_return_date);

      std::string rentalUniqueId = database::DatabaseManager::generateUniqueId("RNT");

      std::string rentalQuery = std::format(
          "INSERT INTO rental (shop_id, cust_id, rental_date, expected_return_date, unique_id) "
          "VALUES ({}, {}, '{}', '{}', '{}')",
          shop_id, cust_id, sqlStart, sqlEnd, rentalUniqueId);

      auto insertRes = db.executeUpdate(rentalQuery);
      if (!insertRes)
        return std::unexpected("Failed to create rental: " + insertRes.error());

      // Get rental_id
      auto id_res = db.executeQuery("SELECT LAST_INSERT_ID() AS last_id");
      if (!id_res)
        return std::unexpected("Failed to retrieve rental ID");

      sql::ResultSet *res = id_res.value();
      int rental_id = -1;
      if (res->next()) {
        rental_id = res->getInt("last_id");
      }
      delete res;

      if (rental_id == -1)
        return std::unexpected("Could not verify rental ID.");

      // 5. Insert into rental_details
      std::string detailsQuery = std::format(
          "INSERT INTO rental_details (rental_id, item_id) "
          "VALUES ({}, {})",
          rental_id, item_id);
      auto detailRes = db.executeUpdate(detailsQuery);
      if (!detailRes)
        return std::unexpected("Failed to create rental details: " +
                               detailRes.error());

      // 6. Insert into invoices
      std::string invoiceUniqueId = database::DatabaseManager::generateUniqueId("INV");

      std::string invoiceQuery = std::format(
          "INSERT INTO invoices (rental_id, base_fee, security_deposit, late_fee, total_amount, payment_status, unique_id) "
          "VALUES ({}, {:.2f}, {:.2f}, 0.00, {:.2f}, 'Paid', '{}')",
          rental_id, total_fee, deposit, total_due, invoiceUniqueId);
      auto invoiceRes = db.executeUpdate(invoiceQuery);
      if (!invoiceRes)
        return std::unexpected("Failed to create invoice: " + invoiceRes.error());

      // 7. Update apparel_item status
      std::string updateItemQuery = std::format(
          "UPDATE apparel_item SET status = 'Rented' WHERE item_id = {}",
          item_id);
      (void)db.executeUpdate(updateItemQuery);

      return rentalUniqueId;
    }

    std::expected<std::vector<RentalHistoryItem>, std::string> getCustomerRentalHistory(int user_id) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string query = std::format(
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
        if (!result) return std::unexpected(result.error());
        
        std::vector<RentalHistoryItem> history;
        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            RentalHistoryItem item;
            item.rental_id = rs->getInt("rental_id");
            item.unique_id = rs->getString("unique_id");
            
            std::string rDate = rs->getString("rental_date");
            item.rental_date = fromMySqlDate(rDate);
            
            std::string eDate = rs->getString("expected_return_date");
            item.expected_return_date = fromMySqlDate(eDate);
            
            std::string actDate = rs->getString("actual_return_date");
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

    std::expected<BookingStats, std::string> getCustomerBookingStats(int user_id) {
        auto& db = database::DatabaseManager::getInstance();
        BookingStats stats;
        stats.return_behaviour = {0, 0, 0, 0};

        // 1. Query Category Popularity
        std::string catQuery = std::format(
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
        std::string retQuery = std::format(
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
        std::string trendQuery = std::format(
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
            std::reverse(stats.monthly_trends.begin(), stats.monthly_trends.end());
            delete rs;
        }

        return stats;
    }

    std::expected<std::vector<ActiveRentalItem>, std::string> getActiveRentals(const std::string& searchTerm) {
        auto& db = database::DatabaseManager::getInstance();
        
        std::string query = 
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
            query += std::format(" AND (cust.fullname LIKE '%{}%' OR c.name LIKE '%{}%')", searchTerm, searchTerm);
        }

        query += " ORDER BY is_overdue DESC, r.expected_return_date ASC";

        auto result = db.executeQuery(query);
        if (!result) return std::unexpected(result.error());

        std::vector<ActiveRentalItem> items;
        sql::ResultSet* rs = result.value();
        while (rs->next()) {
            ActiveRentalItem item;
            item.rental_id = rs->getInt("rental_id");
            item.unique_id = rs->getString("unique_id");
            item.customer_name = rs->getString("customer_name");
            item.item_name = rs->getString("item_name");
            item.size = rs->getString("size");
            
            std::string rDate = rs->getString("rental_date");
            item.rental_date = fromMySqlDate(rDate);
            
            std::string eDate = rs->getString("expected_return_date");
            item.expected_return_date = fromMySqlDate(eDate);

            item.daily_rate = rs->getDouble("daily_rate");
            item.payment_status = rs->getString("payment_status");
            item.is_overdue = (rs->getInt("is_overdue") == 1);
            
            items.push_back(item);
        }
        delete rs;
        return items;
    }

    std::expected<std::string, std::string> processCostumeReturn(
        const std::string& rental_unique_id, 
        const std::string& actual_return_date, 
        const std::string& condition
    ) {
        auto& db = database::DatabaseManager::getInstance();

        // 1. Fetch details of the active rental
        std::string selectQuery = std::format(
            "SELECT r.rental_id, r.unique_id, r.expected_return_date, r.cust_id, cust.user_id, rd.item_id, "
            "       inv.security_deposit, inv.base_fee, c.daily_rate, r.rental_date "
            "FROM rental r "
            "JOIN customers cust ON r.cust_id = cust.cust_id "
            "JOIN rental_details rd ON r.rental_id = rd.rental_id "
            "JOIN apparel_item i ON rd.item_id = i.item_id "
            "JOIN apparel_catalog c ON i.catalog_id = c.catalog_id "
            "JOIN invoices inv ON r.rental_id = inv.rental_id "
            "WHERE (r.unique_id = '{}' OR r.rental_id = '{}') AND r.is_deleted = 0 AND rd.actual_return_date IS NULL LIMIT 1",
            rental_unique_id, rental_unique_id
        );

        auto selectRes = db.executeQuery(selectQuery);
        if (!selectRes) return std::unexpected("Database lookup failed: " + selectRes.error());

        sql::ResultSet* rs = selectRes.value();
        if (!rs->next()) {
            delete rs;
            return std::unexpected("Rental ID either does not exist, has already been returned, or is soft-deleted.");
        }

        int rental_id = rs->getInt("rental_id");
        std::string actual_rental_unique_id = rs->getString("unique_id");
        std::string expected_return_date = rs->getString("expected_return_date");
        int cust_id = rs->getInt("cust_id");
        int user_id = rs->getInt("user_id");
        int item_id = rs->getInt("item_id");
        double security_deposit = rs->getDouble("security_deposit");
        double base_fee = rs->getDouble("base_fee");
        double daily_rate = rs->getDouble("daily_rate");
        delete rs;

        // 2. Date conversion and late fee calculation
        std::string expected_dmY = fromMySqlDate(expected_return_date);
        int lateDays = tool::date::getDaysDifference(expected_dmY, actual_return_date);
        if (lateDays < 0) lateDays = 0;

        // Late fee calculation: standard daily rate * late days
        double lateFee = lateDays * daily_rate;

        // 3. Condition Damage Surcharges
        double damageFee = 0.00;
        if (condition == "Fair") {
            damageFee = 20.00;
        } else if (condition == "Poor" || condition == "Damaged") {
            damageFee = 50.00; // Forfeits standard held deposit amount
        }

        double totalSurcharges = lateFee + damageFee;
        double refund = 0.00;
        double extraCharge = 0.00;

        // 4. Query bank account for settlement
        std::string bankQuery = std::format(
            "SELECT acc_id, balance FROM BANK_ACCOUNT WHERE user_id = {} AND is_deleted = 0 LIMIT 1",
            user_id
        );
        auto bankRes = db.executeQuery(bankQuery);
        int acc_id = -1;
        double active_balance = 0.00;
        if (bankRes) {
            sql::ResultSet* brs = bankRes.value();
            if (brs->next()) {
                acc_id = brs->getInt("acc_id");
                active_balance = brs->getDouble("balance");
            }
            delete brs;
        }

        std::string settlement_status = "Settled";
        std::string payment_method = "Deposit Adjusted";

        if (security_deposit >= totalSurcharges) {
            // Refund the remainder of the deposit
            refund = security_deposit - totalSurcharges;
            if (acc_id != -1 && refund > 0) {
                std::string refundQuery = std::format(
                    "UPDATE BANK_ACCOUNT SET balance = balance + {:.2f} WHERE acc_id = {}",
                    refund, acc_id
                );
                (void)db.executeUpdate(refundQuery);
            }
        } else {
            // Forfeit entire deposit and charge the excess amount
            extraCharge = totalSurcharges - security_deposit;
            if (acc_id != -1) {
                if (active_balance >= extraCharge) {
                    std::string chargeQuery = std::format(
                        "UPDATE BANK_ACCOUNT SET balance = balance - {:.2f} WHERE acc_id = {}",
                        extraCharge, acc_id
                    );
                    (void)db.executeUpdate(chargeQuery);
                    payment_method = "Bank Account / Surcharge";
                } else {
                    // Insufficient funds: deduct down to 0 and mark invoice as Overdue
                    std::string chargeQuery = std::format(
                        "UPDATE BANK_ACCOUNT SET balance = 0.00 WHERE acc_id = {}",
                        acc_id
                    );
                    (void)db.executeUpdate(chargeQuery);
                    settlement_status = "Overdue";
                    payment_method = "Unpaid Surcharge";
                }
            } else {
                settlement_status = "Overdue";
                payment_method = "Unpaid Surcharge";
            }
        }

        // 5. Update rental_details actual return parameters
        std::string sqlActual = toMySqlDate(actual_return_date);
        std::string detailsQuery = std::format(
            "UPDATE rental_details SET actual_return_date = '{}', return_condition = '{}' WHERE rental_id = {}",
            sqlActual, condition, rental_id
        );
        auto detailsRes = db.executeUpdate(detailsQuery);
        if (!detailsRes) return std::unexpected("Failed to update rental details: " + detailsRes.error());

        // 6. Update apparel_item status and condition
        // All returned items go to Laundry first to ensure hygiene and sanitation
        std::string nextStatus = "Laundry";
        std::string itemUpdate = std::format(
            "UPDATE apparel_item SET status = '{}', condition_status = '{}' WHERE item_id = {} AND is_deleted = 0",
            nextStatus, condition, item_id
        );
        auto itemUpdateRes = db.executeUpdate(itemUpdate);
        if (!itemUpdateRes) {
            return std::unexpected("Failed to update apparel item status/condition: " + itemUpdateRes.error());
        }

        // 7. Update invoices financial summary
        double final_total = base_fee + lateFee + damageFee;
        std::string invoiceUpdate = std::format(
            "UPDATE invoices SET late_fee = {:.2f}, total_amount = {:.2f}, "
            "                    payment_status = '{}', payment_date = CURRENT_TIMESTAMP(), "
            "                    payment_method = '{}' "
            "WHERE rental_id = {}",
            lateFee, final_total, settlement_status, payment_method, rental_id
        );
        auto invoiceRes = db.executeUpdate(invoiceUpdate);
        if (!invoiceRes) return std::unexpected("Failed to settle invoice: " + invoiceRes.error());

        // 8. Construct breakdown report for staff review
        std::string depositSettlementMsg;
        if (security_deposit <= 0.00) {
            depositSettlementMsg = "No deposit held.";
        } else if (totalSurcharges == 0.00) {
            if (acc_id != -1) {
                depositSettlementMsg = std::format("RM {:.2f} refunded fully to bank account.", security_deposit);
            } else {
                depositSettlementMsg = std::format("RM {:.2f} settled (no linked bank account to refund).", security_deposit);
            }
        } else if (refund > 0.00) {
            if (acc_id != -1) {
                depositSettlementMsg = std::format("RM {:.2f} refunded (RM {:.2f} forfeited for surcharges).", refund, totalSurcharges);
            } else {
                depositSettlementMsg = std::format("RM {:.2f} settled manually (RM {:.2f} forfeited for surcharges).", refund, totalSurcharges);
            }
        } else if (extraCharge > 0.00) {
            depositSettlementMsg = std::format("Deposit of RM {:.2f} forfeited fully. RM {:.2f} extra charged.", security_deposit, extraCharge);
        } else {
            depositSettlementMsg = std::format("Deposit of RM {:.2f} forfeited fully.", security_deposit);
        }

        std::string report = std::format(
            "\n  --- RETURN FINANCIAL BREAKDOWN ---\n"
            "  Rental Agreement ID : {}\n"
            "  Base Rental Cost    : RM {:.2f}\n"
            "  Held Deposit        : RM {:.2f}\n"
            "  Late Return Days    : {} day(s)\n"
            "  Late Return Surcharge: RM {:.2f}\n"
            "  Damage Surcharge    : RM {:.2f}\n"
            "  ---------------------------------\n"
            "  Total Surcharges    : RM {:.2f}\n"
            "  Deposit Settlement  : {}\n"
            "  Invoice Status      : {}\n",
            actual_rental_unique_id, base_fee, security_deposit, lateDays, lateFee, damageFee,
            totalSurcharges, depositSettlementMsg, settlement_status
        );

        return report;
    }
}
