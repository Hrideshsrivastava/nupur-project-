#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Route.h — a computed journey: the station sequence plus every derived
//  statistic (time, distance, interchanges, fare, crowd, segments).
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/Types.h"

#include <string>
#include <vector>

namespace metro {

/// A maximal run of consecutive hops on the same line (or one walking transfer).
struct Segment {
    int line = -1;                 // -1 => walking transfer
    std::vector<int> stations;     // both endpoints included
    double distanceKm = 0.0;
    double rideMin = 0.0;          // time spent on this segment
    double transferBeforeMin = 0.0; // change penalty paid before boarding

    bool isWalk() const { return line < 0; }
    int stops() const { return static_cast<int>(stations.size()) - 1; }
};

struct Route {
    std::vector<int> stations;  // station ids from source to destination
    std::vector<int> hopLines;  // line used for each hop (size = stations - 1)
    std::vector<Segment> segments;
    std::vector<double> etaMin; // minutes after departure at each station
    std::vector<double> crowd;  // predicted crowd level at each station on arrival

    double distanceKm = 0.0;    // track distance (walks excluded)
    double totalMin = 0.0;      // door-to-door including transfers and walks
    double walkMin = 0.0;
    int interchanges = 0;
    int fare = 0;               // token fare in rupees
    int smartCardFare = 0;      // with the 10% smart-card discount
    double avgCrowd = 0.0;
    double peakCrowd = 0.0;
    int peakCrowdStation = -1;

    std::vector<Criterion> foundBy; // which searches produced this route

    bool empty() const { return stations.size() < 2; }
    int stops() const { return stations.empty() ? 0 : static_cast<int>(stations.size()) - 1; }

    /// A key identifying the exact path (stations + lines), used for de-duplication.
    std::string signature() const;

    /// Kilograms of CO₂ saved compared with driving the same distance by car.
    double co2SavedKg() const;
};

} // namespace metro
