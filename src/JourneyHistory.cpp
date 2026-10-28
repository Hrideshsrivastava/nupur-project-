#include "metro/JourneyHistory.h"

#include "metro/StringUtil.h"

#include <ctime>
#include <fstream>

namespace metro {

JourneyHistory::JourneyHistory(std::string filePath, std::size_t capacity)
    : path_(std::move(filePath)), queue_(capacity) {}

void JourneyHistory::add(const JourneyRecord& record) {
    queue_.enqueue(record);
    save();
}

bool JourneyHistory::load() {
    std::ifstream in(path_);
    if (!in) return false;
    queue_.clear();
    std::string line;
    while (std::getline(in, line)) {
        // One record per line: timestamp|from|to|mode|minutes|fare|interchanges
        const auto f = str::split(line, '|');
        if (f.size() != 7) continue;
        try {
            JourneyRecord r{f[0], f[1], f[2], f[3], std::stod(f[4]), std::stoi(f[5]), std::stoi(f[6])};
            queue_.enqueue(r);
        } catch (...) {
            continue; // skip corrupt lines
        }
    }
    return true;
}

bool JourneyHistory::save() const {
    std::ofstream out(path_, std::ios::trunc);
    if (!out) return false;
    for (std::size_t i = 0; i < queue_.size(); ++i) {
        const JourneyRecord& r = queue_.at(i);
        out << r.timestamp << '|' << r.from << '|' << r.to << '|' << r.mode << '|'
            << str::fixed(r.minutes, 1) << '|' << r.fare << '|' << r.interchanges << '\n';
    }
    return static_cast<bool>(out);
}

void JourneyHistory::clear() {
    queue_.clear();
    save();
}

std::string JourneyHistory::nowStamp() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    char buf[32];
    std::strftime(buf, sizeof buf, "%Y-%m-%d %H:%M", &tm);
    return buf;
}

} // namespace metro
