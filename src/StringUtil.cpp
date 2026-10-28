#include "metro/StringUtil.h"

#include "metro/Types.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace metro::str {

std::string toLower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return s;
}

std::string trim(const std::string& s) {
    std::size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

std::string normalizeKey(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    for (unsigned char c : s)
        if (std::isalnum(c)) out.push_back(static_cast<char>(std::tolower(c)));
    return out;
}

std::string normalizeWords(const std::string& s) {
    std::string out;
    out.reserve(s.size());
    bool pendingSpace = false;
    for (unsigned char c : s) {
        if (std::isalnum(c)) {
            if (pendingSpace && !out.empty()) out.push_back(' ');
            pendingSpace = false;
            out.push_back(static_cast<char>(std::tolower(c)));
        } else {
            pendingSpace = true; // any run of punctuation/space becomes one space
        }
    }
    // A trailing space matters while the user is typing ("noida " != "noida"),
    // so keep one if the input ended with a separator.
    if (pendingSpace && !out.empty() && !s.empty() && s.back() == ' ') out.push_back(' ');
    return out;
}

std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> parts;
    std::string cur;
    for (char c : s) {
        if (c == delimiter) {
            parts.push_back(cur);
            cur.clear();
        } else {
            cur.push_back(c);
        }
    }
    parts.push_back(cur);
    return parts;
}

std::size_t levenshtein(const std::string& a, const std::string& b) {
    // Two-row dynamic programming: O(|a|·|b|) time, O(|b|) memory.
    std::vector<std::size_t> prev(b.size() + 1), cur(b.size() + 1);
    for (std::size_t j = 0; j <= b.size(); ++j) prev[j] = j;
    for (std::size_t i = 1; i <= a.size(); ++i) {
        cur[0] = i;
        for (std::size_t j = 1; j <= b.size(); ++j) {
            std::size_t substitute = prev[j - 1] + (a[i - 1] == b[j - 1] ? 0 : 1);
            cur[j] = std::min({prev[j] + 1, cur[j - 1] + 1, substitute});
        }
        std::swap(prev, cur);
    }
    return prev[b.size()];
}

std::string formatDuration(double minutes) {
    int total = static_cast<int>(std::lround(minutes));
    if (total < 60) return std::to_string(total) + " min";
    std::ostringstream os;
    os << total / 60 << " h " << std::setw(2) << std::setfill('0') << total % 60 << " min";
    return os.str();
}

std::string formatClock(double minuteOfDay) {
    int m = static_cast<int>(std::lround(minuteOfDay));
    m = ((m % kMinutesPerDay) + kMinutesPerDay) % kMinutesPerDay;
    std::ostringstream os;
    os << std::setw(2) << std::setfill('0') << m / 60 << ':' << std::setw(2) << std::setfill('0')
       << m % 60;
    return os.str();
}

bool parseClock(const std::string& text, int& minuteOfDay) {
    std::string t = trim(text);
    if (t.empty()) return false;
    int hours = 0, minutes = 0;
    std::size_t sep = t.find_first_of(":.");
    try {
        if (sep != std::string::npos) {
            std::string h = t.substr(0, sep), m = t.substr(sep + 1);
            if (h.empty() || m.empty() || m.size() > 2) return false;
            for (char c : h + m)
                if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            hours = std::stoi(h);
            minutes = std::stoi(m);
        } else {
            for (char c : t)
                if (!std::isdigit(static_cast<unsigned char>(c))) return false;
            if (t.size() <= 2) {
                hours = std::stoi(t);
            } else if (t.size() <= 4) {
                hours = std::stoi(t.substr(0, t.size() - 2));
                minutes = std::stoi(t.substr(t.size() - 2));
            } else {
                return false;
            }
        }
    } catch (...) {
        return false;
    }
    if (hours < 0 || hours > 23 || minutes < 0 || minutes > 59) return false;
    minuteOfDay = hours * 60 + minutes;
    return true;
}

std::string fixed(double value, int precision) {
    std::ostringstream os;
    os << std::fixed << std::setprecision(precision) << value;
    return os.str();
}

} // namespace metro::str
