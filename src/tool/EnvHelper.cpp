#define _CRT_SECURE_NO_WARNINGS
#include "tool/EnvHelper.h"
#include <fstream>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <cstdlib>

using namespace std;

namespace tool::env {

namespace {
    unordered_map<string, string> envMap;
    bool isLoaded = false;
}

void load(const string& filepath) {
    if (isLoaded) return;
    
    ifstream file(filepath);
    if (!file.is_open()) {
        file.clear();
        file.open("../" + filepath);
        if (!file.is_open()) {
            file.clear();
            file.open("../../" + filepath);
        }
    }

    if (!file.is_open()) {
        return;
    }

    string line;
    while (getline(file, line)) {
        // Strip leading whitespace
        line.erase(line.begin(), find_if(line.begin(), line.end(), [](unsigned char ch) {
            return !isspace(ch);
        }));
        // Strip trailing whitespace
        line.erase(find_if(line.rbegin(), line.rend(), [](unsigned char ch) {
            return !isspace(ch);
        }).base(), line.end());

        // Skip empty or comment lines
        if (line.empty() || line[0] == '#') {
            continue;
        }

        size_t eqPos = line.find('=');
        if (eqPos == string::npos) {
            continue;
        }

        string key = line.substr(0, eqPos);
        string val = line.substr(eqPos + 1);

        // Trim key
        key.erase(key.begin(), find_if(key.begin(), key.end(), [](unsigned char ch) {
            return !isspace(ch);
        }));
        key.erase(find_if(key.rbegin(), key.rend(), [](unsigned char ch) {
            return !isspace(ch);
        }).base(), key.end());

        // Trim val
        val.erase(val.begin(), find_if(val.begin(), val.end(), [](unsigned char ch) {
            return !isspace(ch);
        }));
        val.erase(find_if(val.rbegin(), val.rend(), [](unsigned char ch) {
            return !isspace(ch);
        }).base(), val.end());

        // Strip quotes around value if present
        if (val.size() >= 2 && (
            (val.front() == '"' && val.back() == '"') || 
            (val.front() == '\'' && val.back() == '\'')
        )) {
            val = val.substr(1, val.size() - 2);
        }

        if (!key.empty()) {
            envMap[key] = val;
        }
    }
    isLoaded = true;
}

string get(const string& key, const string& defaultValue) {
    const char* sysVal = getenv(key.c_str());
    if (sysVal != nullptr) {
        return string(sysVal);
    }

    if (!isLoaded) {
        load(".env");
    }

    auto it = envMap.find(key);
    if (it != envMap.end()) {
        return it->second;
    }

    return defaultValue;
}

} // namespace tool::env
