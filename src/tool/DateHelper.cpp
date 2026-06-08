#define _CRT_SECURE_NO_WARNINGS
#include "tool/DateHelper.h"
#include <ctime>
#include <string>

namespace tool::date {

bool parseDateToTm(const std::string &dateStr, std::tm &t) {
  size_t slash1 = dateStr.find('/');
  if (slash1 == std::string::npos) return false;
  size_t slash2 = dateStr.find('/', slash1 + 1);
  if (slash2 == std::string::npos) return false;
  if (dateStr.find('/', slash2 + 1) != std::string::npos) return false;

  std::string dayStr = dateStr.substr(0, slash1);
  std::string monthStr = dateStr.substr(slash1 + 1, slash2 - slash1 - 1);
  std::string yearStr = dateStr.substr(slash2 + 1);

  if (dayStr.empty() || dayStr.length() > 2) return false;
  if (monthStr.empty() || monthStr.length() > 2) return false;
  if (yearStr.length() != 4) return false;

  for (char c : dayStr) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
  }
  for (char c : monthStr) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
  }
  for (char c : yearStr) {
    if (!std::isdigit(static_cast<unsigned char>(c))) return false;
  }

  int day = std::stoi(dayStr);
  int month = std::stoi(monthStr);
  int year = std::stoi(yearStr);

  if (month < 1 || month > 12) return false;

  int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
  if (month == 2) {
    bool isLeap = (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
    if (isLeap) daysInMonth[1] = 29;
  }

  if (day < 1 || day > daysInMonth[month - 1]) return false;

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

std::string normalizeDateStr(const std::string &dateStr) {
  std::tm t;
  if (parseDateToTm(dateStr, t)) {
    char buf[12];
    std::snprintf(buf, sizeof(buf), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    return std::string(buf);
  }
  return dateStr;
}

} // namespace tool::date
