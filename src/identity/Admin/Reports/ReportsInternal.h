#pragma once

#include "transaction/Rental/AdminStats.h"
#include <vector>
#include <string>

namespace stats = ::transaction::rental::stats;

namespace identity::adminui {

    // Shared state update helpers
    bool isValidDate(const std::string& dateStr);
    void updateActiveDateRange(stats::DateRange& activeDateRange, std::string& activeDateRangeLabel);
    void updateActiveFilters(std::vector<int>& activeShopIds, std::vector<std::string>& activeShopNames);

    namespace reports {
        void showMonthlyRevenueReport(
            std::vector<int>& activeShopIds, 
            std::vector<std::string>& activeShopNames,
            stats::DateRange& activeDateRange, 
            std::string& activeDateRangeLabel
        );

        void showCostumePopularityReport(
            std::vector<int>& activeShopIds, 
            std::vector<std::string>& activeShopNames,
            stats::DateRange& activeDateRange, 
            std::string& activeDateRangeLabel
        );

        void showBranchPerformanceReport(
            std::vector<int>& activeShopIds, 
            std::vector<std::string>& activeShopNames,
            stats::DateRange& activeDateRange, 
            std::string& activeDateRangeLabel
        );

        void showInventoryQualityReport(
            std::vector<int>& activeShopIds, 
            std::vector<std::string>& activeShopNames
        );

        void showInvoiceLedgerReport(
            std::vector<int>& activeShopIds, 
            std::vector<std::string>& activeShopNames
        );
    }
}