#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  JourneyHistory.h — the last N planned journeys, kept in a FIFO queue and
//  persisted to a small text file between runs.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/BoundedQueue.h"

#include <string>

namespace metro {

struct JourneyRecord {
    std::string timestamp; // "2026-10-28 09:14"
    std::string from;
    std::string to;
    std::string mode;      // criterion / preference used
    double minutes = 0.0;
    int fare = 0;
    int interchanges = 0;
};

class JourneyHistory {
public:
    explicit JourneyHistory(std::string filePath, std::size_t capacity = 10);

    /// Enqueue a journey (evicting the oldest when full) and save to disk.
    void add(const JourneyRecord& record);

    bool load();       // returns false if the file does not exist / is unreadable
    bool save() const; // returns false if the file cannot be written
    void clear();

    const BoundedQueue<JourneyRecord>& records() const { return queue_; }
    const std::string& path() const { return path_; }

    /// Current local time formatted as "YYYY-MM-DD HH:MM".
    static std::string nowStamp();

private:
    std::string path_;
    BoundedQueue<JourneyRecord> queue_;
};

} // namespace metro
