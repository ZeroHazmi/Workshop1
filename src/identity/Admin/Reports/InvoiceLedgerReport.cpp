#include "identity/Admin/Reports/ReportsInternal.h"
#include "DatabaseManager/DatabaseManager.h"
#include "tool/helper.h"
#include "tool/CLIComponents.h"
#include "tool/input.h"
#include <cppconn/resultset.h>
#include <print>
#include <string>
#include <iostream>
#include <format>
#include <vector>
#include <thread>

using namespace std;

namespace identity::adminui::reports {

    void showInvoiceLedgerReport(
        vector<int>& activeShopIds, 
        vector<string>& activeShopNames
    ) {
        bool inLedger = true;
        string statusFilter = "ALL"; // ALL, Paid, Settled, Overdue
        int currentPage = 1;
        const int itemsPerPage = 15;
        int invalidAttemptsLedger = 0;

        while (inLedger) {
            tool::ui::showHeader("INVOICE LEDGER & AUDITING", 118);

            // Filter Status
            if (activeShopIds.empty()) {
                print("  Active Filter: All Shops");
            } else {
                print("  Active Filter: ");
                for (size_t i = 0; i < activeShopNames.size(); ++i) {
                    print("{}", activeShopNames[i]);
                    if (i + 1 < activeShopNames.size()) print(", ");
                }
            }
            print(" | Status: {}\n", statusFilter);
            println("");

            auto& db = database::DatabaseManager::getInstance();

            // Count total matching records for pagination
            string countQuery = "SELECT COUNT(*) FROM invoices inv JOIN rental r ON inv.rental_id = r.rental_id WHERE 1=1";
            if (!activeShopIds.empty()) {
                string shopFilterStr = "";
                for (size_t i = 0; i < activeShopIds.size(); ++i) {
                    shopFilterStr += to_string(activeShopIds[i]);
                    if (i + 1 < activeShopIds.size()) shopFilterStr += ",";
                }
                countQuery += format(" AND r.shop_id IN ({})", shopFilterStr);
            }
            if (statusFilter != "ALL") {
                countQuery += format(" AND inv.payment_status = '{}'", statusFilter);
            }

            int totalItems = 0;
            auto countRes = db.executeQuery(countQuery);
            if (countRes) {
                sql::ResultSet* rs = countRes.value();
                if (rs->next()) {
                    totalItems = rs->getInt(1);
                }
                delete rs;
            }

            int totalPages = totalItems == 0 ? 1 : (totalItems + itemsPerPage - 1) / itemsPerPage;
            if (currentPage > totalPages) currentPage = totalPages;
            if (currentPage < 1) currentPage = 1;

            // Header
            vector<int> colWidths = {12, 12, 18, 9, 9, 9, 9, 18};
            tool::ui::printRow(colWidths, {"INV ID", "RENTAL ID", "CUSTOMER NAME", "BASE", "DEPOSIT", "LATE FEE", "TOTAL DUE", "STATUS"});
            tool::helper::drawLine(118, '-');

            // Fetch records
            int offset = (currentPage - 1) * itemsPerPage;
            string selectQuery = 
                "SELECT inv.unique_id AS inv_uid, r.unique_id AS rnt_uid, cust.fullname AS customer_name, "
                "       inv.base_fee, inv.security_deposit, inv.late_fee, inv.total_amount, inv.payment_status "
                "FROM invoices inv "
                "JOIN rental r ON inv.rental_id = r.rental_id "
                "JOIN customers cust ON r.cust_id = cust.cust_id "
                "WHERE 1=1";

            if (!activeShopIds.empty()) {
                string shopFilterStr = "";
                for (size_t i = 0; i < activeShopIds.size(); ++i) {
                    shopFilterStr += to_string(activeShopIds[i]);
                    if (i + 1 < activeShopIds.size()) shopFilterStr += ",";
                }
                selectQuery += format(" AND r.shop_id IN ({})", shopFilterStr);
            }
            if (statusFilter != "ALL") {
                selectQuery += format(" AND inv.payment_status = '{}'", statusFilter);
            }
            selectQuery += format(" ORDER BY inv.invoice_id DESC LIMIT {} OFFSET {}", itemsPerPage, offset);

            auto selectRes = db.executeQuery(selectQuery);
            if (selectRes) {
                sql::ResultSet* rs = selectRes.value();
                while (rs->next()) {
                    double base = rs->getDouble("base_fee");
                    double dep = rs->getDouble("security_deposit");
                    double late = rs->getDouble("late_fee");
                    double tot = rs->getDouble("total_amount");
                    string status = rs->getString("payment_status");

                    tool::ui::printRow(colWidths, {
                        rs->getString("inv_uid"),
                        rs->getString("rnt_uid"),
                        rs->getString("customer_name"),
                        format("RM {:.2f}", base),
                        format("RM {:.2f}", dep),
                        format("RM {:.2f}", late),
                        format("RM {:.2f}", tot),
                        status
                    });
                }
                delete rs;
            } else {
                println("  Error fetching invoice ledger: {}", selectRes.error());
            }

            tool::helper::drawLine(118, '=');

            // Pagination details
            tool::ui::printPaginationFooter(currentPage, totalPages, totalItems, 118);
            println("  [F] Filter Status  [0] Back to Reports Gateway");
            tool::helper::drawLine(118, '-');
            print("  Enter selection: ");

            string ledgerInput;
            getline(cin, ledgerInput);

            if (ledgerInput == "0") {
                invalidAttemptsLedger = 0;
                inLedger = false;
            } else if (ledgerInput == "N" || ledgerInput == "n") {
                invalidAttemptsLedger = 0;
                if (currentPage < totalPages) currentPage++;
            } else if (ledgerInput == "P" || ledgerInput == "p") {
                invalidAttemptsLedger = 0;
                if (currentPage > 1) currentPage--;
            } else if (ledgerInput == "F" || ledgerInput == "f") {
                invalidAttemptsLedger = 0;
                println("\n  Select Status Filter:");
                println("  [1] All Invoices");
                println("  [2] Paid (Active Rentals)");
                println("  [3] Settled (Completed Returns)");
                println("  [4] Overdue (Outstanding Surcharges)");
                print("  Enter choice: ");
                string fChoice;
                getline(cin, fChoice);
                if (fChoice == "1") statusFilter = "ALL";
                else if (fChoice == "2") statusFilter = "Paid";
                else if (fChoice == "3") statusFilter = "Settled";
                else if (fChoice == "4") statusFilter = "Overdue";
                currentPage = 1;
            } else {
                println("  Invalid choice.");
                if (!tool::ui::handleInvalidAttempt(invalidAttemptsLedger)) {
                    this_thread::sleep_for(chrono::milliseconds(1000));
                }
            }
        }
    }
}
