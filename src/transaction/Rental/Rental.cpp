#include "DatabaseManager/DatabaseManager.h"
#include "transaction/Rental/Rental.h"
#include <format>
#include <iostream>

namespace transaction::rental {
    std::string toMySqlDate(const std::string &date) {
      // DD/MM/YYYY to YYYY-MM-DD
      if (date.length() != 10)
        return date;
      return date.substr(6, 4) + "-" + date.substr(3, 2) + "-" +
             date.substr(0, 2);
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

      // 3. Insert into rental
      std::string sqlStart = toMySqlDate(start_date);
      std::string sqlEnd = toMySqlDate(expected_return_date);
      double deposit = 0.00; // On hold per user request
      double total_fee = daily_rate * total_days;

      std::string rentalQuery = std::format(
          "INSERT INTO rental (shop_id, cust_id, rental_date, expected_return_date) "
          "VALUES ({}, {}, '{}', '{}')",
          shop_id, cust_id, sqlStart, sqlEnd);

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

      // 4. Insert into rental_details
      std::string detailsQuery = std::format(
          "INSERT INTO rental_details (rental_id, item_id) "
          "VALUES ({}, {})",
          rental_id, item_id);
      auto detailRes = db.executeUpdate(detailsQuery);
      if (!detailRes)
        return std::unexpected("Failed to create rental details: " +
                               detailRes.error());

      // 5. Insert into invoices
      std::string invoiceQuery = std::format(
          "INSERT INTO invoices (rental_id, base_fee, security_deposit, late_fee, total_amount, payment_status) "
          "VALUES ({}, {:.2f}, {:.2f}, 0.00, {:.2f}, 'Pending')",
          rental_id, total_fee, deposit, total_fee);
      auto invoiceRes = db.executeUpdate(invoiceQuery);
      if (!invoiceRes)
        return std::unexpected("Failed to create invoice: " + invoiceRes.error());

      // 6. Update apparel_item status
      std::string updateItemQuery = std::format(
          "UPDATE apparel_item SET status = 'Rented' WHERE item_id = {}",
          item_id);
      (void)db.executeUpdate(updateItemQuery);

      return std::format("TX-{:04d}", rental_id);
    }
}
