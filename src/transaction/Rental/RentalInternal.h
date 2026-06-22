#pragma once
#include <string>

namespace transaction::rental {
    inline std::string toMySqlDate(const std::string &date) {
        if (date.length() != 10) return date;
        return date.substr(6, 4) + "-" + date.substr(3, 2) + "-" + date.substr(0, 2);
    }

    inline std::string fromMySqlDate(const std::string &date) {
        if (date.length() != 10) return date;
        return date.substr(8, 2) + "/" + date.substr(5, 2) + "/" + date.substr(0, 4);
    }
}
