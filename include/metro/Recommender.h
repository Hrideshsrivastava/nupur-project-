#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Recommender.h — intelligent route recommendation.
//
//  1. Generate candidates with every criterion (5×Dijkstra + BFS) plus
//     alternative corridors.
//  2. De-duplicate identical paths.
//  3. Award badges (FASTEST, CHEAPEST, …) to the best route in each metric.
//  4. Score every route with preference-dependent weights on min-max
//     normalised metrics and rank them with a min-heap.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/RouteFinder.h"

#include <string>
#include <vector>

namespace metro {

struct ScoredRoute {
    Route route;
    double score = 0.0;              // lower is better (0 = best in every metric)
    int match = 0;                   // 0..100 "match %" shown to the user
    std::vector<std::string> badges; // e.g. "FASTEST", "CHEAPEST"
};

struct Reason {
    bool positive; // true = advantage, false = heads-up / warning
    std::string text;
};

struct Recommendation {
    std::vector<ScoredRoute> options; // ranked, best first
    std::vector<Reason> reasons;      // why options[0] was chosen
    double computeMs = 0.0;           // wall time spent searching
    std::size_t statesExplored = 0;   // total states settled by all searches
};

/// Relative importance of each metric for a preference (weights sum to 1).
struct Weights {
    double time, interchanges, fare, crowd, distance;
};

class Recommender {
public:
    explicit Recommender(const RouteFinder& finder);

    Recommendation recommend(int source, int target, const JourneyContext& ctx,
                             Preference preference) const;

    static Weights weightsFor(Preference preference);

private:
    const RouteFinder& finder_;
};

} // namespace metro
