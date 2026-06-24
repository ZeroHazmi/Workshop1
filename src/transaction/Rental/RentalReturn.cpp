#include "transaction/Rental/Rental.h"
#include "transaction/Rental/RentalInternal.h"
#include "DatabaseManager/DatabaseManager.h"
#include "tool/DateHelper.h"
#include <cppconn/resultset.h>
#include <format>
#include <algorithm>
#include <expected>

namespace db = ::database;

using namespace std;

namespace transaction::rental {

    expected<string, string> processCostumeReturn(
        const string& rental_unique_id, 
        const string& actual_return_date, 
        const string& condition
    ) {
        auto& db = db::DatabaseManager::getInstance();

        // 1. Fetch details of the active rental
        string selectQuery = format(
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
        if (!selectRes) return unexpected("Database lookup failed: " + selectRes.error());

        sql::ResultSet* rs = selectRes.value();
        if (!rs->next()) {
            delete rs;
            return unexpected("Rental ID either does not exist, has already been returned, or is soft-deleted.");
        }

        int rental_id = rs->getInt("rental_id");
        string actual_rental_unique_id = rs->getString("unique_id");
        string expected_return_date = rs->getString("expected_return_date");
        int cust_id = rs->getInt("cust_id");
        int user_id = rs->getInt("user_id");
        int item_id = rs->getInt("item_id");
        double security_deposit = rs->getDouble("security_deposit");
        double base_fee = rs->getDouble("base_fee");
        double daily_rate = rs->getDouble("daily_rate");
        delete rs;

        // 2. Date conversion and late fee calculation
        string expected_dmY = fromMySqlDate(expected_return_date);
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
        string bankQuery = format(
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

        string settlement_status = "Settled";
        string payment_method = "Deposit Adjusted";

        if (security_deposit >= totalSurcharges) {
            // Refund the remainder of the deposit
            refund = security_deposit - totalSurcharges;
            if (acc_id != -1 && refund > 0) {
                string refundQuery = format(
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
                    string chargeQuery = format(
                        "UPDATE BANK_ACCOUNT SET balance = balance - {:.2f} WHERE acc_id = {}",
                        extraCharge, acc_id
                    );
                    (void)db.executeUpdate(chargeQuery);
                    payment_method = "Bank Account / Surcharge";
                } else {
                    // Insufficient funds: deduct down to 0 and mark invoice as Overdue
                    string chargeQuery = format(
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
        string sqlActual = toMySqlDate(actual_return_date);
        string detailsQuery = format(
            "UPDATE rental_details SET actual_return_date = '{}', return_condition = '{}' WHERE rental_id = {}",
            sqlActual, condition, rental_id
        );
        auto detailsRes = db.executeUpdate(detailsQuery);
        if (!detailsRes) return unexpected("Failed to update rental details: " + detailsRes.error());

        // 6. Update apparel_item status and condition
        // All returned items go to Laundry first to ensure hygiene and sanitation,
        // except those in Fair, Poor, or Damaged condition, which are routed to Maintenance.
        string nextStatus = "Laundry";
        if (condition == "Fair" || condition == "Poor" || condition == "Damaged") {
            nextStatus = "Maintenance";
        }
        string itemUpdate = format(
            "UPDATE apparel_item SET status = '{}', condition_status = '{}' WHERE item_id = {} AND is_deleted = 0",
            nextStatus, condition, item_id
        );
        auto itemUpdateRes = db.executeUpdate(itemUpdate);
        if (!itemUpdateRes) {
            return unexpected("Failed to update apparel item status/condition: " + itemUpdateRes.error());
        }

        // 7. Update invoices financial summary
        double final_total = base_fee + lateFee + damageFee;
        string invoiceUpdate = format(
            "UPDATE invoices SET late_fee = {:.2f}, total_amount = {:.2f}, "
            "                    payment_status = '{}', payment_date = CURRENT_TIMESTAMP(), "
            "                    payment_method = '{}' "
            "WHERE rental_id = {}",
            lateFee, final_total, settlement_status, payment_method, rental_id
        );
        auto invoiceRes = db.executeUpdate(invoiceUpdate);
        if (!invoiceRes) return unexpected("Failed to settle invoice: " + invoiceRes.error());

        // 8. Construct breakdown report for staff review
        string depositSettlementMsg;
        if (security_deposit <= 0.00) {
            depositSettlementMsg = "No deposit held.";
        } else if (totalSurcharges == 0.00) {
            if (acc_id != -1) {
                depositSettlementMsg = format("RM {:.2f} refunded fully to bank account.", security_deposit);
            } else {
                depositSettlementMsg = format("RM {:.2f} settled (no linked bank account to refund).", security_deposit);
            }
        } else if (refund > 0.00) {
            if (acc_id != -1) {
                depositSettlementMsg = format("RM {:.2f} refunded (RM {:.2f} forfeited for surcharges).", refund, totalSurcharges);
            } else {
                depositSettlementMsg = format("RM {:.2f} settled manually (RM {:.2f} forfeited for surcharges).", refund, totalSurcharges);
            }
        } else if (extraCharge > 0.00) {
            depositSettlementMsg = format("Deposit of RM {:.2f} forfeited fully. RM {:.2f} extra charged.", security_deposit, extraCharge);
        } else {
            depositSettlementMsg = format("Deposit of RM {:.2f} forfeited fully.", security_deposit);
        }

        string report = format(
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