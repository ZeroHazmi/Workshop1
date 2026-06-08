#pragma once

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/resultset.h>
#include <cppconn/statement.h>

#include <string>
#include <string_view>
#include <expected>
#include <memory>

namespace database {

class DatabaseManager {
public:
    // Singleton Access
    static DatabaseManager& getInstance() {
        static DatabaseManager instance;
        return instance;
    }

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    std::expected<void, std::string> connect();
    std::expected<sql::ResultSet*, std::string> executeQuery(std::string_view query);
    std::expected<int, std::string> executeUpdate(std::string_view query);
    
    // Alphanumeric Unique ID Generator
    static std::string generateUniqueId(std::string_view prefix);

private:
    DatabaseManager() = default;
    ~DatabaseManager();

    sql::Driver* driver = nullptr;
    std::unique_ptr<sql::Connection> con;

    // Credentials matching your existing MySQL setup
    const std::string host = "tcp://127.0.0.1:3306";
    const std::string user = "root";
    const std::string pass = "Testing184"; 
    const std::string schema = "clothing_rental";
};

} // namespace database