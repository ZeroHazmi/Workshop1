cmake_minimum_required(VERSION 3.10)
project(FWCRS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set the path to where you extracted the MySQL Connector
set(MYSQL_CON_DIR "C:/Program Files/MySQL/mysql-connector-c++-9.7.0")

include_directories("${MYSQL_CON_DIR}/include")
include_directories("${MYSQL_CON_DIR}/include/jdbc")
link_directories("${MYSQL_CON_DIR}/lib64/vs14")

# Source files
set(SOURCES
	src/main.cpp
	src/identity/Profile/Profile.cpp
	src/identity/Auth/Auth.cpp
	src/identity/Customer/Customer.cpp
	src/identity/Staff/Staff.cpp
	src/identity/Admin/AdminDashboard.cpp
	src/identity/Admin/AdminStaffUI.cpp
	src/identity/Admin/AdminShopUI.cpp
	src/identity/Admin/AdminReportsUI.cpp
	src/identity/Admin/Reports/MonthlyRevenueReport.cpp
	src/identity/Admin/Reports/CostumePopularityReport.cpp
	src/identity/Admin/Reports/BranchPerformanceReport.cpp
	src/identity/Admin/Reports/InventoryQualityReport.cpp
	src/identity/Admin/Reports/InvoiceLedgerReport.cpp
	src/identity/Customer/CustomerDashboard.cpp
	src/identity/Customer/CustomerProfileUI.cpp
	src/identity/Customer/CustomerBankUI.cpp
	src/identity/Customer/CustomerRentalHistoryUI.cpp
	src/identity/Staff/StaffDashboard.cpp
	src/identity/Staff/StaffInventoryUI.cpp
	src/identity/Staff/StaffRentalsUI.cpp
	src/inventory/Apparel/Apparel.cpp
	src/inventory/InventoryCatalogUI.cpp
	src/inventory/InventoryCheckoutUI.cpp
	src/inventory/InventoryRegisterUI.cpp
	src/transaction/Rental/RentalCreation.cpp
	src/transaction/Rental/RentalReturn.cpp
	src/transaction/Rental/RentalQueries.cpp
	src/transaction/Rental/AdminStats.cpp
	src/DatabaseManager/DatabaseManager.cpp
	src/tool/helper.cpp
	src/tool/EnvHelper.cpp
	src/tool/DateHelper.cpp
	src/tool/CLIComponents.cpp
	src/tool/input.cpp
)

include(CTest)
enable_testing()

add_executable(fwcrs ${SOURCES})

# Include directories for headers
target_include_directories(fwcrs PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/include
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/tool
)

target_link_libraries(fwcrs PRIVATE mysqlcppconn)

add_custom_command(TARGET fwcrs POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${MYSQL_CON_DIR}/lib64/mysqlcppconn-10-vs14.dll"
    "${MYSQL_CON_DIR}/lib64/libcrypto-3-x64.dll"
    "${MYSQL_CON_DIR}/lib64/libssl-3-x64.dll"
    $<TARGET_FILE_DIR:fwcrs>
)

message(STATUS "Configured FWCRS project with ${CMAKE_CXX_STANDARD} standard")

