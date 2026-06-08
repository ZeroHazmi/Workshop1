#include "DatabaseManager/DatabaseManager.h"
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
    try {
        driver = get_driver_instance();
        con.reset(driver->connect(host, user, pass));
        con->setSchema(schema);
        
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

std::string DatabaseManager::generateUniqueId(string_view prefix) {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<> dis(0, 35);
    const char chars[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    
    std::string suffix;
    for (int i = 0; i < 6; ++i) {
        suffix += chars[dis(gen)];
    }
    return std::format("{}-{}", prefix, suffix);
}

} // namespace database