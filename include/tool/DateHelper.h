#pragma once

#include <string>

namespace tool::date {

    // Checks if the string matches DD/MM/YYYY and has valid days/months
    bool isValidFormat(const std::string& dateStr);

    // Checks if the given date is before today's date
    bool isBeforeToday(const std::string& dateStr);

    // Calculates difference in days (endDate - startDate)
    int getDaysDifference(const std::string& startDate, const std::string& endDate);

    // Normalizes dynamic date format (e.g. 6/3/2026 -> 06/03/2026)
    std::string normalizeDateStr(const std::string& dateStr);
}
