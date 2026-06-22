#define _CRT_SECURE_NO_WARNINGS
#include "tool/DateHelper.h"
#include <ctime>
#include <string>

using namespace std;

namespace tool::date {

bool parseDateToTm(const string &dateStr, tm &t) {
  size_t slash1 = dateStr.find('/');
  if (slash1 == string::npos) return false;
  size_t slash2 = dateStr.find('/', slash1 + 1);
  if (slash2 == string::npos) return false;
  if (dateStr.find('/', slash2 + 1) != string::npos) return false;

  string dayStr = dateStr.substr(0, slash1);
  string monthStr = dateStr.substr(slash1 + 1, slash2 - slash1 - 1);
  string yearStr = dateStr.substr(slash2 + 1);

  if (dayStr.empty() || dayStr.length() > 2) return false;
  if (monthStr.empty() || monthStr.length() > 2) return false;
  if (yearStr.length() != 4) return false;

  for (char c : dayStr) {
    if (!isdigit(static_cast<unsigned char>(c))) return false;
  }
  for (char c : monthStr) {
    if (!isdigit(static_cast<unsigned char>(c))) return false;
  }
  for (char c : yearStr) {
    if (!isdigit(static_cast<unsigned char>(c))) return false;
  }

  int day = stoi(dayStr);
  int month = stoi(monthStr);
  int year = stoi(yearStr);

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

bool isValidFormat(const string &dateStr) {
  tm t;
  return parseDateToTm(dateStr, t);
}

bool isBeforeToday(const string &dateStr) {
  tm t;
  if (!parseDateToTm(dateStr, t))
    return false; // Invalid format acts as 'before today' or error

  time_t checkTime = mktime(&t);

  time_t now = time(nullptr);
  tm *nowTm = localtime(&now);
  // Normalize today to midnight
  nowTm->tm_hour = 0;
  nowTm->tm_min = 0;
  nowTm->tm_sec = 0;
  time_t todayTime = mktime(nowTm);

  // Return true if the given date is strictly less than today (midnight)
  return difftime(checkTime, todayTime) < 0;
}

int getDaysDifference(const string &startDate,
                      const string &endDate) {
  tm startTm, endTm;
  if (!parseDateToTm(startDate, startTm) || !parseDateToTm(endDate, endTm))
    return -1;

  time_t start = mktime(&startTm);
  time_t end = mktime(&endTm);

  if (start == -1 || end == -1)
    return -1;

  double seconds = difftime(end, start);
  return static_cast<int>(seconds / (60.0 * 60.0 * 24.0));
}

string normalizeDateStr(const string &dateStr) {
  tm t;
  if (parseDateToTm(dateStr, t)) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%02d/%02d/%04d", t.tm_mday, t.tm_mon + 1, t.tm_year + 1900);
    return string(buf);
  }
  return dateStr;
}

}