#include "metro/RouteFinder.h"

#include "metro/FareCalculator.h"
#include "metro/MinHeap.h"

#include <algorithm>
#include <climits>
#include <limits>
#include <map>
#include <queue>
#include <tuple>
#include <unordered_set>

namespace metro {

namespace {
constexpr double kInf = std::numeric_limits<double>::infinity();
constexpr double kAlternativePenalty = 1.8; // multiplier on edges already used
} // namespace

RouteFinder::RouteFinder(const MetroGraph& graph, const CrowdModel& crowd)
    : graph_(graph), crowd_(crowd) {}

std::uint64_t RouteFinder::edgeKey(int a, int b) {
    const auto lo = static_cast<std::uint64_t>(std::min(a, b));
    const auto hi = static_cast<std::uint64_t>(std::max(a, b));
    return (lo << 32) | hi;
}

std::optional<Route> RouteFinder::find(int source, int target, Criterion criterion,
                                       const JourneyContext& ctx) const {
    if (criterion == Criterion::FewestStops) return fewestStops(source, target, ctx);
    return dijkstra(source, target, criterion, ctx, nullptr);
}

double RouteFinder::edgeCost(const Edge& edge, bool lineChange, bool boardingPremium,
                             Criterion criterion, double stepMinutes, double clockMinute,
                             DayType day) const {
    switch (criterion) {
        case Criterion::Fastest:
            return stepMinutes;
        case Criterion::Shortest:
            // Distance first; time only breaks ties between equal distances.
            return edge.distanceKm + 0.001 * stepMinutes;
        case Criterion::LeastInterchange:
            return (lineChange ? 1000.0 : 0.0) + stepMinutes;
        case Criterion::Cheapest: {
            // Fares grow with distance; premium lines cost ~3x per km plus a
            // boarding charge. Time breaks ties.
            double fareWeight = 0.0;
            if (!edge.isWalk()) {
                const bool premium = graph_.line(edge.line).premium;
                fareWeight = edge.distanceKm * (premium ? 3.0 : 1.0) + (boardingPremium ? 12.0 : 0.0);
            }
            return fareWeight + 0.001 * stepMinutes;
        }
        case Criterion::LeastCrowded: {
            // Crowd at the station we are about to reach, at the time we reach it.
            const double crowd = crowd_.level(edge.to, clockMinute, day) / 100.0;
            return stepMinutes * (1.0 + 2.5 * crowd * crowd);
        }
        case Criterion::FewestStops:
            return 1.0;
    }
    return stepMinutes;
}

std::optional<Route> RouteFinder::dijkstra(int source, int target, Criterion criterion,
                                           const JourneyContext& ctx,
                                           const EdgePenalties* penalties) const {
    explored_ = 0;
    const int n = static_cast<int>(graph_.stationCount());
    if (source < 0 || target < 0 || source >= n || target >= n || source == target)
        return std::nullopt;

    // State = station * slots + (line + 1); slot 0 means "not on a train yet".
    const int slots = static_cast<int>(graph_.lineCount()) + 1;
    const std::size_t states = static_cast<std::size_t>(n) * static_cast<std::size_t>(slots);
    std::vector<double> best(states, kInf);
    std::vector<int> parent(states, -1);
    std::vector<int> arrivedOn(states, -2); // line of the edge used to arrive

    struct Label {
        double cost;
        double elapsed; // real minutes since departure (for time-of-day crowd)
        int state;
    };
    auto byCost = [](const Label& a, const Label& b) { return a.cost < b.cost; };
    MinHeap<Label, decltype(byCost)> frontier(byCost);

    const int start = source * slots;
    best[static_cast<std::size_t>(start)] = 0.0;
    frontier.push(Label{0.0, 0.0, start});
    int goal = -1;

    while (!frontier.empty()) {
        const Label cur = frontier.pop();
        if (cur.cost > best[static_cast<std::size_t>(cur.state)]) continue; // stale entry
        ++explored_;

        const int u = cur.state / slots;
        const int currentLine = cur.state % slots - 1;
        if (u == target) {
            goal = cur.state;
            break;
        }

        for (const Edge& e : graph_.neighbors(u)) {
            const bool walk = e.isWalk();
            // Walking keeps the "last train line" so the next boarding is a change.
            const int nextLine = walk ? currentLine : e.line;
            const bool change = !walk && currentLine >= 0 && e.line != currentLine;
            const bool boardingPremium =
                !walk && graph_.line(e.line).premium &&
                (currentLine < 0 || !graph_.line(currentLine).premium);

            const double step = e.timeMin + (change ? kTransferPenaltyMin : 0.0);
            const double elapsed = cur.elapsed + step;
            double w = edgeCost(e, change, boardingPremium, criterion, step,
                                ctx.departMinute + elapsed, ctx.day);
            if (penalties) {
                auto it = penalties->find(edgeKey(u, e.to));
                if (it != penalties->end()) w *= it->second;
            }

            const int next = e.to * slots + (nextLine + 1);
            const double candidate = cur.cost + w;
            if (candidate < best[static_cast<std::size_t>(next)]) {
                best[static_cast<std::size_t>(next)] = candidate;
                parent[static_cast<std::size_t>(next)] = cur.state;
                arrivedOn[static_cast<std::size_t>(next)] = e.line;
                frontier.push(Label{candidate, elapsed, next});
            }
        }
    }

    if (goal < 0) return std::nullopt;

    // Walk the parent pointers back to the source.
    std::vector<int> stations, hopLines;
    for (int s = goal; s != -1; s = parent[static_cast<std::size_t>(s)]) {
        stations.push_back(s / slots);
        if (parent[static_cast<std::size_t>(s)] != -1) hopLines.push_back(arrivedOn[static_cast<std::size_t>(s)]);
    }
    std::reverse(stations.begin(), stations.end());
    std::reverse(hopLines.begin(), hopLines.end());

    Route route = buildRoute(stations, hopLines, ctx);
    route.foundBy.push_back(criterion);
    return route;
}

std::optional<Route> RouteFinder::fewestStops(int source, int target,
                                              const JourneyContext& ctx) const {
    explored_ = 0;
    const int n = static_cast<int>(graph_.stationCount());
    if (source < 0 || target < 0 || source >= n || target >= n || source == target)
        return std::nullopt;

    // Classic BFS: the first time we dequeue the target we have the fewest hops.
    std::vector<int> parent(static_cast<std::size_t>(n), -1);
    std::vector<bool> seen(static_cast<std::size_t>(n), false);
    std::queue<int> q;
    q.push(source);
    seen[static_cast<std::size_t>(source)] = true;
    while (!q.empty()) {
        const int u = q.front();
        q.pop();
        ++explored_;
        if (u == target) break;
        for (const Edge& e : graph_.neighbors(u)) {
            if (seen[static_cast<std::size_t>(e.to)]) continue;
            seen[static_cast<std::size_t>(e.to)] = true;
            parent[static_cast<std::size_t>(e.to)] = u;
            q.push(e.to);
        }
    }
    if (!seen[static_cast<std::size_t>(target)]) return std::nullopt;

    std::vector<int> path;
    for (int v = target; v != -1; v = parent[static_cast<std::size_t>(v)]) path.push_back(v);
    std::reverse(path.begin(), path.end());

    Route route = buildRoute(path, assignLines(path), ctx);
    route.foundBy.push_back(Criterion::FewestStops);
    return route;
}

std::vector<int> RouteFinder::assignLines(const std::vector<int>& path) const {
    // Dynamic programming over hops. State = last train line ridden (-1 none).
    // Value = (line changes so far, previous state, line chosen for this hop).
    const std::size_t hops = path.empty() ? 0 : path.size() - 1;
    std::vector<std::map<int, std::tuple<int, int, int>>> layer(hops + 1);
    layer[0][-1] = {0, INT_MIN, INT_MIN};

    for (std::size_t i = 0; i < hops; ++i) {
        for (const auto& [lastLine, info] : layer[i]) {
            for (const Edge& e : graph_.neighbors(path[i])) {
                if (e.to != path[i + 1]) continue;
                const int newLast = e.isWalk() ? lastLine : e.line;
                const int changes =
                    std::get<0>(info) + ((!e.isWalk() && lastLine >= 0 && e.line != lastLine) ? 1 : 0);
                auto it = layer[i + 1].find(newLast);
                if (it == layer[i + 1].end() || changes < std::get<0>(it->second))
                    layer[i + 1][newLast] = {changes, lastLine, e.line};
            }
        }
    }

    std::vector<int> lines(hops, -1);
    if (hops == 0 || layer[hops].empty()) return lines;
    int state = layer[hops].begin()->first;
    for (const auto& [key, info] : layer[hops])
        if (std::get<0>(info) < std::get<0>(layer[hops][state])) state = key;
    for (std::size_t i = hops; i > 0; --i) {
        const auto& info = layer[i].at(state);
        lines[i - 1] = std::get<2>(info);
        state = std::get<1>(info);
    }
    return lines;
}

std::vector<Route> RouteFinder::alternatives(int source, int target, const JourneyContext& ctx,
                                             int count) const {
    std::vector<Route> result;
    auto base = dijkstra(source, target, Criterion::Fastest, ctx, nullptr);
    if (!base) return result;

    // Penalise the edges of every route found so far and search again: the
    // next search is pushed towards genuinely different corridors.
    EdgePenalties penalties;
    std::unordered_set<std::string> seen{base->signature()};
    auto penalise = [&](const Route& r) {
        for (std::size_t i = 0; i + 1 < r.stations.size(); ++i) {
            double& m = penalties[edgeKey(r.stations[i], r.stations[i + 1])];
            m = (m == 0.0 ? 1.0 : m) * kAlternativePenalty;
        }
    };
    penalise(*base);

    for (int attempt = 0; attempt < count * 4 && static_cast<int>(result.size()) < count; ++attempt) {
        auto alt = dijkstra(source, target, Criterion::Fastest, ctx, &penalties);
        if (!alt) break;
        penalise(*alt);
        // Keep only distinct routes that are not absurdly slower than the best.
        if (seen.insert(alt->signature()).second && alt->totalMin <= base->totalMin * 1.5 + 20.0) {
            alt->foundBy.clear();
            result.push_back(*alt);
        }
    }
    return result;
}

Route RouteFinder::buildRoute(const std::vector<int>& stations, const std::vector<int>& hopLines,
                              const JourneyContext& ctx) const {
    Route r;
    r.stations = stations;
    r.hopLines = hopLines;
    r.etaMin.assign(stations.size(), 0.0);

    double clock = 0.0;
    int lastTrainLine = -1;
    for (std::size_t i = 0; i + 1 < stations.size(); ++i) {
        const int line = hopLines[i];
        const Edge* e = graph_.edgeBetween(stations[i], stations[i + 1], line);
        if (!e) continue; // should not happen for paths produced by the finder

        const bool change = line >= 0 && lastTrainLine >= 0 && line != lastTrainLine;
        double transfer = 0.0;
        if (change) {
            ++r.interchanges;
            transfer = kTransferPenaltyMin;
            clock += transfer;
        }
        // Start a new segment whenever the line (or walk/ride mode) changes.
        if (r.segments.empty() || r.segments.back().line != line) {
            Segment seg;
            seg.line = line;
            seg.stations.push_back(stations[i]);
            seg.transferBeforeMin = transfer;
            r.segments.push_back(seg);
        }
        Segment& seg = r.segments.back();
        seg.stations.push_back(stations[i + 1]);
        seg.rideMin += e->timeMin;
        clock += e->timeMin;
        r.etaMin[i + 1] = clock;

        if (line >= 0) {
            seg.distanceKm += e->distanceKm;
            r.distanceKm += e->distanceKm;
            lastTrainLine = line;
        } else {
            seg.distanceKm += e->distanceKm;
            r.walkMin += e->timeMin;
        }
    }
    r.totalMin = clock;

    const FareBreakdown fare = FareCalculator::compute(r, graph_, ctx.day);
    r.fare = fare.total;
    r.smartCardFare = fare.smartCard;

    // Predicted crowd at every station at the moment the passenger is there.
    r.crowd.resize(stations.size());
    double sum = 0.0;
    for (std::size_t i = 0; i < stations.size(); ++i) {
        const double c = crowd_.level(stations[i], ctx.departMinute + r.etaMin[i], ctx.day);
        r.crowd[i] = c;
        sum += c;
        if (c > r.peakCrowd || r.peakCrowdStation < 0) {
            r.peakCrowd = c;
            r.peakCrowdStation = stations[i];
        }
    }
    r.avgCrowd = stations.empty() ? 0.0 : sum / static_cast<double>(stations.size());
    return r;
}

} // namespace metro
