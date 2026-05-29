#define _CRT_SECURE_NO_WARNINGS
#include "tool/DateHelper.h"
#include <ctime>
#include <string>

namespace tool::date {

bool parseDateToTm(const std::string &dateStr, std::tm &t) {
  if (dateStr.length() != 10 || dateStr[2] != '/' || dateStr[5] != '/')
    return false;

  int day, month, year;
  try {
    day = std::stoi(dateStr.substr(0, 2));
    month = std::stoi(dateStr.substr(3, 2));
    year = std::stoi(dateStr.substr(6, 4));
  } catch (...) {
    return false;
  }

  if (day < 1 || day > 31 || month < 1 || month > 12)
    return false;

  t = {};
  t.tm_mday = day;
  t.tm_mon = month - 1;
  t.tm_year = year - 1900;
  return true;
}

bool isValidFormat(const std::string &dateStr) {
  std::tm t;
  return parseDateToTm(dateStr, t);
}

bool isBeforeToday(const std::string &dateStr) {
  std::tm t;
  if (!parseDateToTm(dateStr, t))
    return false; // Invalid format acts as 'before today' or error

  std::time_t checkTime = std::mktime(&t);

  std::time_t now = std::time(nullptr);
  std::tm *nowTm = std::localtime(&now);
  // Normalize today to midnight
  nowTm->tm_hour = 0;
  nowTm->tm_min = 0;
  nowTm->tm_sec = 0;
  std::time_t todayTime = std::mktime(nowTm);

  // Return true if the given date is strictly less than today (midnight)
  return std::difftime(checkTime, todayTime) < 0;
}

int getDaysDifference(const std::string &startDate,
                      const std::string &endDate) {
  std::tm startTm, endTm;
  if (!parseDateToTm(startDate, startTm) || !parseDateToTm(endDate, endTm))
    return -1;

  std::time_t start = std::mktime(&startTm);
  std::time_t end = std::mktime(&endTm);

  if (start == -1 || end == -1)
    return -1;

  double seconds = std::difftime(end, start);
  return static_cast<int>(seconds / (60.0 * 60.0 * 24.0));
}

} // namespace tool::date
