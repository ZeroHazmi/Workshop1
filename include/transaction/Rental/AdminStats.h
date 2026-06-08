#pragma once

#include <string>
#include <vector>
#include <expected>

namespace transaction::rental::stats {

    struct DateRange {
        std::string startDate; // YYYY-MM-DD
        std::string endDate;   // YYYY-MM-DD
    };

    struct RevenueTrendPoint {
        std::string month_name;
        double revenue;
    };

    struct CostumePopularityPoint {
        std::string catalog_name;
        int rental_count;
    };

    struct BranchPerformancePoint {
        std::string shop_name;
        double revenue;
    };

    struct InventoryConditionPoint {
        std::string condition;
        int count;
    };

    std::expected<std::vector<RevenueTrendPoint>, std::string> getRevenueTrends(
        const std::vector<int>& shopIds = {}, 
        const DateRange& dateRange = {}
    );
    
    std::expected<std::vector<CostumePopularityPoint>, std::string> getCostumePopularity(
        const std::vector<int>& shopIds = {}, 
        const DateRange& dateRange = {}
    );
    
    std::expected<std::vector<BranchPerformancePoint>, std::string> getBranchPerformance(
        const std::vector<int>& shopIds = {}, 
        const DateRange& dateRange = {}
    );
    
    std::expected<std::vector<InventoryConditionPoint>, std::string> getInventoryConditionAudit(
        const std::vector<int>& shopIds = {}
    );

} // namespace transaction::rental::stats
