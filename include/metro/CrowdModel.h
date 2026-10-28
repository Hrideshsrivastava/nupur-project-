#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  CrowdModel.h — peak-hour crowd prediction for every station.
//
//  crowd(station, time) = popularity(station) × demand(time of day) + jitter
//  where demand follows a predefined weekday / Sunday hourly profile with
//  morning (08–10) and evening (17–19) peaks.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/MetroGraph.h"

#include <vector>

namespace metro {

class CrowdModel {
public:
    explicit CrowdModel(const MetroGraph& graph);

    /// Predicted crowd level 0..100 at `station` at `minuteOfDay` (may exceed 1440).
    double level(int station, double minuteOfDay, DayType day) const;

    /// Network-wide demand 0..1 at a given time, linearly interpolated per hour.
    static double demand(double minuteOfDay, DayType day);

    /// The 24 hourly predictions for a station (index = hour).
    std::vector<double> forecast(int station, DayType day) const;

    /// True during the morning/evening rush on weekdays.
    static bool isPeak(double minuteOfDay, DayType day);

    /// "Low", "Moderate", "High" or "Packed".
    static const char* label(double level);

private:
    const MetroGraph& graph_;
};

} // namespace metro
