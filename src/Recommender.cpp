#include "metro/Recommender.h"

#include "metro/MinHeap.h"
#include "metro/StringUtil.h"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <unordered_map>

namespace metro {

namespace {

constexpr double kEps = 1e-9;

// Min-max normalisation: best value -> 0, worst -> 1.
double normalise(double value, double lo, double hi) {
    return hi - lo < kEps ? 0.0 : (value - lo) / (hi - lo);
}

// The metrics used to compare routes (lower is better for all of them).
double timeOf(const Route& r) { return r.totalMin; }
double changesOf(const Route& r) { return r.interchanges; }
double fareOf(const Route& r) { return r.fare; }
double crowdOf(const Route& r) { return r.avgCrowd; }
double distOf(const Route& r) { return r.distanceKm; }
double stopsOf(const Route& r) { return r.stops(); }

// Lowest and highest value of one metric across all routes.
struct Range {
    double lo;
    double hi;
};

Range rangeOf(const std::vector<Route>& routes, double (*metric)(const Route&)) {
    Range range{metric(routes.front()), metric(routes.front())};
    for (const Route& r : routes) {
        range.lo = std::min(range.lo, metric(r));
        range.hi = std::max(range.hi, metric(r));
    }
    return range;
}

} // namespace

Recommender::Recommender(const RouteFinder& finder) : finder_(finder) {}

Weights Recommender::weightsFor(Preference preference) {
    switch (preference) {
        case Preference::Balanced: return {0.35, 0.20, 0.15, 0.20, 0.10};
        case Preference::Hurry:    return {0.70, 0.10, 0.05, 0.10, 0.05};
        case Preference::Budget:   return {0.15, 0.10, 0.60, 0.10, 0.05};
        case Preference::Comfort:  return {0.20, 0.20, 0.05, 0.50, 0.05};
        case Preference::Simple:   return {0.20, 0.60, 0.05, 0.10, 0.05};
    }
    return {0.35, 0.20, 0.15, 0.20, 0.10};
}

Recommendation Recommender::recommend(int source, int target, const JourneyContext& ctx,
                                      Preference preference) const {
    const auto started = std::chrono::steady_clock::now();
    Recommendation rec;

    // ── 1. Candidate generation ──────────────────────────────────────────────
    std::vector<Route> candidates;
    std::unordered_map<std::string, std::size_t> bySignature;
    auto addCandidate = [&](Route route) {
        auto it = bySignature.find(route.signature());
        if (it != bySignature.end()) { // same path found by another search
            auto& found = candidates[it->second].foundBy;
            found.insert(found.end(), route.foundBy.begin(), route.foundBy.end());
            return;
        }
        bySignature.emplace(route.signature(), candidates.size());
        candidates.push_back(std::move(route));
    };

    const Criterion all[] = {Criterion::Fastest,      Criterion::Shortest,
                             Criterion::LeastInterchange, Criterion::Cheapest,
                             Criterion::LeastCrowded, Criterion::FewestStops};
    for (Criterion c : all) {
        Route r = finder_.find(source, target, c, ctx);
        if (!r.empty()) addCandidate(r);
        rec.statesExplored += finder_.lastExplored();
    }
    for (Route& alt : finder_.alternatives(source, target, ctx, 2)) addCandidate(std::move(alt));
    rec.statesExplored += finder_.lastExplored();

    if (candidates.empty()) return rec;

    // ── 2. Metric ranges for normalisation + badges ──────────────────────────
    const Range timeRange = rangeOf(candidates, timeOf);
    const Range changeRange = rangeOf(candidates, changesOf);
    const Range fareRange = rangeOf(candidates, fareOf);
    const Range crowdRange = rangeOf(candidates, crowdOf);
    const Range distRange = rangeOf(candidates, distOf);
    const Range stopRange = rangeOf(candidates, stopsOf);

    // ── 3. Score + rank with a min-heap ──────────────────────────────────────
    const Weights w = weightsFor(preference);
    auto byScore = [](const ScoredRoute& a, const ScoredRoute& b) {
        if (std::fabs(a.score - b.score) > kEps) return a.score < b.score;
        return a.route.totalMin < b.route.totalMin;
    };
    MinHeap<ScoredRoute, decltype(byScore)> heap(byScore);

    for (Route& r : candidates) {
        ScoredRoute s;
        s.score = w.time * normalise(timeOf(r), timeRange.lo, timeRange.hi) +
                  w.interchanges * normalise(changesOf(r), changeRange.lo, changeRange.hi) +
                  w.fare * normalise(fareOf(r), fareRange.lo, fareRange.hi) +
                  w.crowd * normalise(crowdOf(r), crowdRange.lo, crowdRange.hi) +
                  w.distance * normalise(distOf(r), distRange.lo, distRange.hi);
        s.match = static_cast<int>(std::lround(100.0 * (1.0 - s.score)));

        if (timeOf(r) - timeRange.lo < 0.05) s.badges.push_back("FASTEST");
        if (changesOf(r) - changeRange.lo < kEps) s.badges.push_back(r.interchanges == 0 ? "DIRECT" : "FEWEST CHANGES");
        if (fareOf(r) - fareRange.lo < kEps) s.badges.push_back("CHEAPEST");
        if (crowdOf(r) - crowdRange.lo < 0.05) s.badges.push_back("LEAST CROWDED");
        if (distOf(r) - distRange.lo < 0.05) s.badges.push_back("SHORTEST");
        if (stopsOf(r) - stopRange.lo < kEps) s.badges.push_back("FEWEST STOPS");

        s.route = std::move(r);
        heap.push(std::move(s));
    }
    while (!heap.empty()) rec.options.push_back(heap.pop());

    // ── 4. Explain the recommendation ────────────────────────────────────────
    const ScoredRoute& best = rec.options.front();
    const Route& b = best.route;
    auto has = [&](const char* badge) {
        return std::find(best.badges.begin(), best.badges.end(), badge) != best.badges.end();
    };

    if (has("FASTEST")) {
        rec.reasons.push_back({true, "Quickest option: arrive in " + str::formatDuration(b.totalMin)});
    } else {
        rec.reasons.push_back({true, "Only " + str::formatDuration(b.totalMin - timeRange.lo) +
                                         " slower than the fastest route, but a better overall fit"});
    }
    if (b.interchanges == 0)
        rec.reasons.push_back({true, "Direct ride — no line changes needed"});
    else if (has("FEWEST CHANGES"))
        rec.reasons.push_back({true, "Fewest line changes possible (" + std::to_string(b.interchanges) + ")"});
    if (has("CHEAPEST"))
        rec.reasons.push_back({true, "Lowest fare: ₹" + std::to_string(b.fare) + " (₹" +
                                         std::to_string(b.smartCardFare) + " with smart card)"});
    if (has("LEAST CROWDED") && rec.options.size() > 1)
        rec.reasons.push_back({true, "Least crowded corridor: avg load " +
                                         std::to_string(static_cast<int>(std::lround(b.avgCrowd))) + "%"});
    if (b.peakCrowd >= 75 && b.peakCrowdStation >= 0)
        rec.reasons.push_back({false, "Heavy crowd expected at " +
                                          finder_.graph().station(b.peakCrowdStation).name + " (~" +
                                          std::to_string(static_cast<int>(std::lround(b.peakCrowd))) + "%)"});
    if (CrowdModel::isPeak(ctx.departMinute, ctx.day))
        rec.reasons.push_back({false, "You are travelling in peak hours — allow a few extra minutes"});

    rec.computeMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - started).count();
    return rec;
}

} // namespace metro
