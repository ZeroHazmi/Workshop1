#include "DatabaseManager.h"
#include <print>

namespace database {

DatabaseManager::~DatabaseManager() {
    if (con) con->close();
}

std::expected<void, std::string> DatabaseManager::connect() {
    try {
        driver = get_driver_instance();
        con.reset(driver->connect(host, user, pass));
        con->setSchema(schema);
        
        std::println("Successfully connected to database database.");
        return {}; 
    } catch (sql::SQLException& e) {
        std::string err = "Connection Failed: " + std::string(e.what());
        std::println(stderr, "{}", err);
        return std::unexpected(err);
    }
}

std::expected<sql::ResultSet*, std::string> DatabaseManager::executeQuery(std::string_view query) {
    try {
        if (!con || con->isClosed()) return std::unexpected("Connection lost.");

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        // Caller must manage the lifetime of the returned ResultSet
        return stmt->executeQuery(std::string(query));
    } catch (sql::SQLException& e) {
        return std::unexpected("Query Error: " + std::string(e.what()));
    }
}

std::expected<int, std::string> DatabaseManager::executeUpdate(std::string_view query) {
    try {
        if (!con || con->isClosed()) return std::unexpected("Connection lost.");

        std::unique_ptr<sql::Statement> stmt(con->createStatement());
        return stmt->executeUpdate(std::string(query));
    } catch (sql::SQLException& e) {
        return std::unexpected("Update Error: " + std::string(e.what()));
    }
}

} // namespace database