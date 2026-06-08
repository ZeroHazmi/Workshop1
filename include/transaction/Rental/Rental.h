#pragma once

#include <string>
#include <vector>
#include <expected>

struct Rental {
    int rental_id; // Primary Key
    std::string unique_id;
    int shop_id;   // Foreign Key -> Shops.shop_id
    int cust_id;   // Foreign Key -> Customers.cust_id
    std::string rental_date;
    std::string expected_return_date;
    bool is_deleted;
};

namespace transaction::rental {

    struct RentalHistoryItem {
        int rental_id;
        std::string unique_id;
        std::string rental_date;
        std::string expected_return_date;
        std::string actual_return_date;
        std::string item_name;
        std::string size;
        double base_fee;
        double late_fee;
        std::string payment_status;
        std::string return_condition;
    };

    struct ActiveRentalItem {
        int rental_id;
        std::string unique_id;
        std::string customer_name;
        std::string item_name;
        std::string size;
        std::string rental_date;
        std::string expected_return_date;
        double daily_rate;
        std::string payment_status;
        bool is_overdue;
    };

    struct CategoryStat {
        std::string category;
        int count;
    };

    struct ReturnStats {
        int on_time;
        int late;
        int active_on_time;
        int active_overdue;
    };

    struct MonthlyStat {
        std::string month_name;
        int count;
    };

    struct BookingStats {
        std::vector<CategoryStat> categories;
        ReturnStats return_behaviour;
        std::vector<MonthlyStat> monthly_trends;
    };

    std::expected<std::string, std::string> createRental(
        int user_id, 
        int catalog_id, 
        const std::string& size, 
        const std::string& start_date, 
        const std::string& expected_return_date, 
        double daily_rate, 
        int total_days
    );

    std::expected<std::vector<RentalHistoryItem>, std::string> getCustomerRentalHistory(int user_id);

    std::expected<BookingStats, std::string> getCustomerBookingStats(int user_id);

    std::expected<std::vector<ActiveRentalItem>, std::string> getActiveRentals(const std::string& searchTerm = "");

    std::expected<std::string, std::string> processCostumeReturn(
        const std::string& rental_unique_id, 
        const std::string& actual_return_date, 
        const std::string& condition
    );

}
