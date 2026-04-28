cmake_minimum_required(VERSION 3.10)
project(FWCRS VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Source files
set(SOURCES
	src/main.cpp
	src/identity/Users/Users.cpp
	src/identity/Profile/Profile.cpp
	src/inventory/Shops/Shops.cpp
	src/inventory/Apparel/Apparel.cpp
	src/transaction/Rental_Details/Rental_Details.cpp
	src/transaction/Rental/Rental.cpp
	src/DatabaseManager/DatabaseManager.cpp
	tool/tool.cpp
)

include(CTest)
enable_testing()

add_executable(fwcrs ${SOURCES})

# Include directories for headers
target_include_directories(fwcrs PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src
	${CMAKE_CURRENT_SOURCE_DIR}/tool
)

message(STATUS "Configured FWCRS project with ${CMAKE_CXX_STANDARD} standard")

