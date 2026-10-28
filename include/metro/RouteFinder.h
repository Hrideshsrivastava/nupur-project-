#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  RouteFinder.h — the path-finding engine.
//
//  Dijkstra runs on an expanded state space (station, line you arrived on) so
//  that changing lines can be charged correctly: the same station reached on
//  two different lines is two different states. BFS answers "fewest stops".
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/CrowdModel.h"
#include "metro/MetroGraph.h"
#include "metro/Route.h"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace metro {

/// When and on what kind of day the passenger travels.
struct JourneyContext {
    int departMinute = 9 * 60;
    DayType day = DayType::Weekday;
};

/// Multipliers applied to station-pair edges; used to discover alternatives.
using EdgePenalties = std::unordered_map<std::uint64_t, double>;

class RouteFinder {
public:
    RouteFinder(const MetroGraph& graph, const CrowdModel& crowd);

    /// Best route for one criterion, or nullopt if unreachable / same station.
    std::optional<Route> find(int source, int target, Criterion criterion,
                              const JourneyContext& ctx) const;

    /// Route with the fewest stations in between (Breadth-First Search).
    std::optional<Route> fewestStops(int source, int target, const JourneyContext& ctx) const;

    /// Up to `count` alternatives to the fastest route (edge-penalty method).
    std::vector<Route> alternatives(int source, int target, const JourneyContext& ctx,
                                    int count) const;

    /// Turn a station sequence + hop lines into a fully annotated Route.
    Route buildRoute(const std::vector<int>& stations, const std::vector<int>& hopLines,
                     const JourneyContext& ctx) const;

    const MetroGraph& graph() const { return graph_; }

    /// Number of states settled by the most recent search (for statistics).
    std::size_t lastExplored() const { return explored_; }

private:
    std::optional<Route> dijkstra(int source, int target, Criterion criterion,
                                  const JourneyContext& ctx, const EdgePenalties* penalties) const;

    double edgeCost(const Edge& edge, bool lineChange, bool boardingPremium, Criterion criterion,
                    double stepMinutes, double clockMinute, DayType day) const;

    /// Pick a line for every hop of a station path minimising line changes.
    std::vector<int> assignLines(const std::vector<int>& path) const;

    static std::uint64_t edgeKey(int a, int b);

    const MetroGraph& graph_;
    const CrowdModel& crowd_;
    mutable std::size_t explored_ = 0;
};

} // namespace metro
