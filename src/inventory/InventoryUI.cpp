#include "inventory/InventoryUI.h"
#include "tool/CLIComponents.h"
#include "tool/helper.h"
#include <vector>
#include <iostream>
#include <print>

using namespace std;

namespace inventory::ui {

    
    void showCatalog() {
    tool::helper::clearScreen();
    vector<int> colWidths = {4, 25, 12, 10};
    
    tool::ui::displayTitle("APPAREL CATALOG", 65);
    // Header Row
    tool::ui::printRow(colWidths, {"ID", "ITEM NAME", "CATEGORY", "PRICE/DAY"});
    tool::helper::drawLine(65, '-');

    // Data Rows (Fetch these from your Database ResultSet)
    tool::ui::printRow(colWidths, {"001", "Baju Melayu (Satin)", "Formal", "RM 45.00"});
    tool::ui::printRow(colWidths, {"002", "Tuxedo Black (Slim)", "Costume", "RM 60.00"});
    
    tool::helper::drawLine(65, '=');
    println("\nPress Enter to return to dashboard...");
    cin.ignore(10000, '\n');
    }
}