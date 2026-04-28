#include "DatabaseManager.h"

DatabaseManager::DatabaseManager() : driver(nullptr) {
    try {
        driver = get_driver_instance();
    } catch (sql::SQLException& e) {
        std::cerr << "Could not get a database driver instance: " << e.what() << std::endl;
    }
}

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

bool DatabaseManager::connect() {
    try {
        std::cout << "[Step 1] Initializing Driver..." << std::endl;
        sql::mysql::MySQL_Driver *mysql_driver = sql::mysql::get_mysql_driver_instance();
        
        if (!mysql_driver) {
            std::cerr << "Could not get MySQL driver instance." << std::endl;
            return false;
        }

        std::cout << "[Step 2] Connecting..." << std::endl;
        // Use a raw pointer first to test
        sql::Connection* rawCon = mysql_driver->connect("tcp://127.0.0.1:3306", "root", "Testing184");
        
        if (!rawCon) {
            std::cerr << "Connection failed!" << std::endl;
            return false;
        }

        con.reset(rawCon);
        con->setSchema("clothing_rental");

        std::cout << "Successfully connected!" << std::endl;
        return true;
    } catch (sql::SQLException &e) {
        std::cerr << "MySQL Error: " << e.what() << " (Code: " << e.getErrorCode() << ")" << std::endl;
        return false;
    }
}

sql::Connection* DatabaseManager::getConnection() {
    return con.get();
}