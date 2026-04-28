#pragma once

#include <mysql_driver.h>
#include <mysql_connection.h>
#include <cppconn/driver.h>
#include <cppconn/exception.h>
#include <cppconn/prepared_statement.h>
#include <memory>
#include <iostream>

class DatabaseManager {
private:
    sql::Driver* driver;
    std::unique_ptr<sql::Connection> con;

    // Private constructor for Singleton pattern
    DatabaseManager();

public:
    // Delete copy constructor and assignment operator
    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    static DatabaseManager& getInstance();
    
    bool connect();
    sql::Connection* getConnection();
    
    ~DatabaseManager() = default;
};