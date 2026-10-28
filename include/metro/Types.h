#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Types.h — shared enums, constants and the RGB colour type used everywhere.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstdint>
#include <string>

namespace metro {

/// A 24-bit colour. Every metro line owns one; the UI derives gradients from it.
struct RGB {
    std::uint8_t r = 255;
    std::uint8_t g = 255;
    std::uint8_t b = 255;
};

/// Minutes lost when changing lines (platform walk + average wait for a train).
constexpr double kTransferPenaltyMin = 6.0;
/// Average halt time at every station a train passes through.
constexpr double kDwellMin = 0.5;
/// Minutes in one day; used to wrap clock arithmetic.
constexpr int kMinutesPerDay = 24 * 60;

/// What the passenger wants to optimise when searching for a single route.
enum class Criterion {
    Fastest,          // Dijkstra on travel time (+ transfer penalty)
    Shortest,         // Dijkstra on track distance
    LeastInterchange, // Dijkstra with a huge cost per line change
    Cheapest,         // Dijkstra on fare-related weights
    LeastCrowded,     // Dijkstra on crowd-weighted travel time
    FewestStops       // Breadth-first search (unweighted hops)
};

/// The passenger's overall priority, used by the recommender to rank routes.
enum class Preference { Balanced, Hurry, Budget, Comfort, Simple };

/// Fares and crowd levels differ between working days and Sundays/holidays.
enum class DayType { Weekday, Sunday };

inline const char* criterionName(Criterion c) {
    switch (c) {
        case Criterion::Fastest:          return "Fastest";
        case Criterion::Shortest:         return "Shortest";
        case Criterion::LeastInterchange: return "Least Interchange";
        case Criterion::Cheapest:         return "Cheapest";
        case Criterion::LeastCrowded:     return "Least Crowded";
        case Criterion::FewestStops:      return "Fewest Stops";
    }
    return "?";
}

inline const char* criterionAlgorithm(Criterion c) {
    switch (c) {
        case Criterion::Fastest:          return "Dijkstra · weight = travel time";
        case Criterion::Shortest:         return "Dijkstra · weight = distance (km)";
        case Criterion::LeastInterchange: return "Dijkstra · weight = 1000 x line changes + time";
        case Criterion::Cheapest:         return "Dijkstra · weight = fare per km";
        case Criterion::LeastCrowded:     return "Dijkstra · weight = time x predicted crowd";
        case Criterion::FewestStops:      return "Breadth-First Search · unweighted hops";
    }
    return "?";
}

inline const char* preferenceName(Preference p) {
    switch (p) {
        case Preference::Balanced: return "Balanced";
        case Preference::Hurry:    return "In a hurry";
        case Preference::Budget:   return "On a budget";
        case Preference::Comfort:  return "Comfort first";
        case Preference::Simple:   return "Keep it simple";
    }
    return "?";
}

inline const char* preferenceTagline(Preference p) {
    switch (p) {
        case Preference::Balanced: return "a smart mix of time, changes, fare and crowd";
        case Preference::Hurry:    return "minimise door-to-door travel time";
        case Preference::Budget:   return "minimise the fare";
        case Preference::Comfort:  return "avoid crowded stations and trains";
        case Preference::Simple:   return "as few line changes as possible";
    }
    return "";
}

inline const char* dayTypeName(DayType d) {
    return d == DayType::Weekday ? "Weekday" : "Sunday / Holiday";
}

} // namespace metro
