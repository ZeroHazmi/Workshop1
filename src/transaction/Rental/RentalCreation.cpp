#include "transaction/Rental/Rental.h"
#include "transaction/Rental/RentalInternal.h"
#include "DatabaseManager/DatabaseManager.h"
#include <cppconn/resultset.h>
#include <format>
#include <algorithm>
#include <expected>

namespace transaction::rental {

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

}
