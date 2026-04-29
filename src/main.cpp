#include <print>
#include <string>
#include <string_view>
#include <expected>
#include <thread>
#include <chrono>
#include <iostream>
#include "DatabaseManager/DatabaseManager.h"

using namespace std;



int main() {
	cout << "FWCRS starting..." << std::endl;

	if (!database::DatabaseManager::getInstance().connect()) {
        std::cerr << "Application failed to start: Database connection error." << std::endl;
        return 1;
    }

	return 0;
}

