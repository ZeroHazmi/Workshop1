#include "DatabaseManager/DatabaseManager.h"
#include "tool/EnvHelper.h"
#include <print>
#include <random>
#include <format>

using namespace sql;
using namespace std;

namespace database {

DatabaseManager::~DatabaseManager() {
    if (con) con->close();
}

expected<void, string> DatabaseManager::connect() {
    host = tool::env::get("DB_HOST", "tcp://127.0.0.1:3306");
    user = tool::env::get("DB_USER", "root");
    pass = tool::env::get("DB_PASS", "");
    schema = tool::env::get("DB_SCHEMA", "clothing_rental");

    try {
        driver = get_driver_instance();
        con.reset(driver->connect(host, user, pass));
        con->setSchema(schema);
        
        // Auto-correct any pre-existing/legacy items with Fair, Poor, or Damaged condition to Maintenance status
        unique_ptr<Statement> stmt(con->createStatement());
        stmt->executeUpdate(
            "UPDATE apparel_item SET status = 'Maintenance' "
            "WHERE condition_status IN ('Fair', 'Poor', 'Damaged') "
            "  AND status != 'Maintenance' AND is_deleted = 0"
        );
        
        println("Successfully connected to database database.");
        return {}; 
    } catch (SQLException& e) {
        string err = "Connection Failed: " + string(e.what());
        println(stderr, "{}", err);
        return unexpected(err);
    }
}

expected<ResultSet*, string> DatabaseManager::executeQuery(string_view query) {
    try {
        if (!con || con->isClosed()) return unexpected("Connection lost.");

        unique_ptr<Statement> stmt(con->createStatement());
        // Caller must manage the lifetime of the returned ResultSet
        return stmt->executeQuery(string(query));
    } catch (SQLException& e) {
        return unexpected("Query Error: " + string(e.what()));
    }
}

expected<int, string> DatabaseManager::executeUpdate(string_view query) {
    try {
        if (!con || con->isClosed()) return unexpected("Connection lost.");

        unique_ptr<Statement> stmt(con->createStatement());
        return stmt->executeUpdate(string(query));
    } catch (SQLException& e) {
        return unexpected("Update Error: " + string(e.what()));
    }
}

string DatabaseManager::generateUniqueId(string_view prefix) {
    static random_device rd;
    static mt19937 gen(rd());
    static uniform_int_distribution<> dis(0, 35);
    const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    string suffix;
    for (int i = 0; i < 6; ++i) {
        suffix += chars[dis(gen)];
    }
    return format("{}-{}", prefix, suffix);
}

} // namespace database