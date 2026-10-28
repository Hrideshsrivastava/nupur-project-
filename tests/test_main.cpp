// ─────────────────────────────────────────────────────────────────────────────
//  test_main.cpp — dependency-free unit and integration tests (Phase 5).
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/BoundedQueue.h"
#include "metro/CrowdModel.h"
#include "metro/FareCalculator.h"
#include "metro/JourneyHistory.h"
#include "metro/MetroData.h"
#include "metro/MinHeap.h"
#include "metro/Recommender.h"
#include "metro/RouteFinder.h"
#include "metro/Sorting.h"
#include "metro/StringUtil.h"
#include "metro/Trie.h"

#include <cstdio>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

namespace {

int g_failed = 0;
int g_checks = 0;

#define CHECK(cond)                                                                       \
    do {                                                                                  \
        ++g_checks;                                                                       \
        if (!(cond)) {                                                                    \
            ++g_failed;                                                                   \
            std::cout << "    \x1b[31mFAILED\x1b[0m " << #cond << "  (" << __FILE__ << ":" \
                      << __LINE__ << ")\n";                                               \
        }                                                                                 \
    } while (0)

void run(const char* name, const std::function<void()>& test) {
    const int before = g_failed;
    test();
    std::cout << (g_failed == before ? "  \x1b[32m✔\x1b[0m " : "  \x1b[31m✖\x1b[0m ") << name << "\n";
}

using namespace metro;

const MetroGraph& graph() {
    static const MetroGraph g = data::buildDelhiMetro();
    return g;
}

int id(const char* name) { return graph().findStation(name); }

} // namespace

int main() {
    std::cout << "\nMetro Route Navigation System — test suite\n\n";
    const CrowdModel crowd(graph());
    const RouteFinder finder(graph(), crowd);
    const JourneyContext ctx{9 * 60, DayType::Weekday};

    run("MinHeap pops in ascending order", [] {
        MinHeap<int> h;
        for (int v : {5, 1, 9, 3, 7, 2, 8}) h.push(v);
        std::vector<int> out;
        while (!h.empty()) out.push_back(h.pop());
        CHECK((out == std::vector<int>{1, 2, 3, 5, 7, 8, 9}));
    });

    run("BoundedQueue evicts the oldest element (FIFO)", [] {
        BoundedQueue<int> q(3);
        CHECK(!q.enqueue(1));
        q.enqueue(2);
        q.enqueue(3);
        CHECK(q.enqueue(4)); // evicts 1
        CHECK(q.size() == 3);
        CHECK(q.front() == 2 && q.back() == 4);
        CHECK(q.dequeue() == 2);
        CHECK(q.at(0) == 3);
    });

    run("mergeSort sorts and is stable", [] {
        std::vector<std::pair<int, char>> v = {{3, 'a'}, {1, 'b'}, {3, 'c'}, {2, 'd'}, {1, 'e'}};
        mergeSort(v, [](const auto& a, const auto& b) { return a.first < b.first; });
        CHECK((v == std::vector<std::pair<int, char>>{{1, 'b'}, {1, 'e'}, {2, 'd'}, {3, 'a'}, {3, 'c'}}));
    });

    run("String helpers: normalise, Levenshtein, clock parsing", [] {
        CHECK(str::normalizeKey("Dilli Haat - INA") == "dillihaatina");
        CHECK(str::normalizeWords("  Dilli   Haat - INA") == "dilli haat ina");
        CHECK(str::levenshtein("kitten", "sitting") == 3);
        int m = 0;
        CHECK(str::parseClock("09:30", m) && m == 570);
        CHECK(str::parseClock("1745", m) && m == 1065);
        CHECK(!str::parseClock("25:00", m));
        CHECK(!str::parseClock("ab", m));
        CHECK(str::formatClock(1440 + 5) == "00:05");
    });

    run("Trie autocomplete: prefix, mid-word and ranking", [] {
        Trie t;
        for (const Station& s : graph().stations()) t.insert(s.name, s.id);
        auto r = t.suggest("rajiv", 5);
        CHECK(!r.empty() && r.front() == id("Rajiv Chowk"));
        auto sec = t.suggest("sector 62", 5);
        CHECK(!sec.empty() && sec.front() == id("Noida Sector 62"));
        auto chowk = t.suggest("chowk", 10);
        CHECK(chowk.size() >= 4); // Rajiv, Chandni, IFFCO, Patel, …
        CHECK(t.suggest("zzzz", 5).empty());
        CHECK(t.contains("Kashmere Gate"));
    });

    run("Graph: hash lookup, connectivity, interchanges", [] {
        CHECK(id("rajiv   CHOWK") == id("Rajiv Chowk"));
        CHECK(id("Not A Station") == -1);
        CHECK(graph().stationCount() > 240);
        CHECK(graph().isConnected());
        CHECK(graph().station(id("Kashmere Gate")).lines.size() == 3);
        CHECK(graph().station(id("Noida Sector 52")).isInterchange()); // skywalk
    });

    run("Fare slabs", [] {
        CHECK(FareCalculator::slabFare(1.5, DayType::Weekday) == 11);
        CHECK(FareCalculator::slabFare(10, DayType::Weekday) == 32);
        CHECK(FareCalculator::slabFare(40, DayType::Weekday) == 64);
        CHECK(FareCalculator::slabFare(10, DayType::Sunday) == 21);
        CHECK(FareCalculator::premiumFare(100) == 75);
    });

    run("Crowd model peaks at rush hour", [&] {
        const int rc = id("Rajiv Chowk");
        CHECK(crowd.level(rc, 9 * 60, DayType::Weekday) > crowd.level(rc, 14 * 60, DayType::Weekday));
        CHECK(crowd.level(rc, 3 * 60, DayType::Weekday) < 5);
        CHECK(crowd.level(rc, 9 * 60, DayType::Weekday) > crowd.level(id("Dhansa Bus Stand"), 9 * 60, DayType::Weekday));
        CHECK(CrowdModel::isPeak(18 * 60, DayType::Weekday));
        CHECK(!CrowdModel::isPeak(18 * 60, DayType::Sunday));
    });

    run("Dijkstra: direct ride on one line has 0 interchanges", [&] {
        auto r = finder.find(id("Rajiv Chowk"), id("Noida Sector 62"), Criterion::Fastest, ctx);
        CHECK(r.has_value());
        CHECK(r && r->interchanges == 0);
        CHECK(r && r->stations.front() == id("Rajiv Chowk") && r->stations.back() == id("Noida Sector 62"));
        CHECK(r && r->segments.size() == 1);
    });

    run("Dijkstra: cross-network route needs a change", [&] {
        auto r = finder.find(id("Samaypur Badli"), id("Botanical Garden"), Criterion::Fastest, ctx);
        CHECK(r && r->interchanges >= 1);
        CHECK(r && r->totalMin > 0 && r->distanceKm > 0);
        // Every hop must be a real edge of the graph.
        for (std::size_t i = 0; r && i + 1 < r->stations.size(); ++i)
            CHECK(graph().edgeBetween(r->stations[i], r->stations[i + 1], r->hopLines[i]) != nullptr);
    });

    run("Each criterion is optimal for its own metric", [&] {
        const int a = id("Rithala"), b = id("Noida Electronic City");
        auto fastest = finder.find(a, b, Criterion::Fastest, ctx);
        auto shortest = finder.find(a, b, Criterion::Shortest, ctx);
        auto fewest = finder.find(a, b, Criterion::LeastInterchange, ctx);
        auto stops = finder.find(a, b, Criterion::FewestStops, ctx);
        auto cheap = finder.find(a, b, Criterion::Cheapest, ctx);
        CHECK(fastest && shortest && fewest && stops && cheap);
        for (const auto* r : {&shortest, &fewest, &stops, &cheap}) CHECK(fastest->totalMin <= (*r)->totalMin + 1e-9);
        for (const auto* r : {&fastest, &fewest, &stops, &cheap}) CHECK(shortest->distanceKm <= (*r)->distanceKm + 1e-9);
        for (const auto* r : {&fastest, &shortest, &stops, &cheap}) CHECK(fewest->interchanges <= (*r)->interchanges);
        for (const auto* r : {&fastest, &shortest, &fewest, &cheap}) CHECK(stops->stops() <= (*r)->stops());
        for (const auto* r : {&fastest, &shortest, &fewest, &stops}) CHECK(cheap->fare <= (*r)->fare);
    });

    run("Skywalk transfer to the Aqua Line", [&] {
        auto r = finder.find(id("Rajiv Chowk"), id("Pari Chowk"), Criterion::Fastest, ctx);
        CHECK(r.has_value());
        bool walked = false;
        for (const Segment& s : r->segments) walked = walked || s.isWalk();
        CHECK(walked);
        CHECK(r->interchanges == 1);
    });

    run("Invalid queries return no route", [&] {
        CHECK(!finder.find(id("Rajiv Chowk"), id("Rajiv Chowk"), Criterion::Fastest, ctx));
        CHECK(!finder.find(-1, id("Rajiv Chowk"), Criterion::Fastest, ctx));
        CHECK(!finder.find(id("Rajiv Chowk"), 99999, Criterion::FewestStops, ctx));
    });

    run("Alternatives are distinct from the fastest route", [&] {
        auto base = finder.find(id("Kashmere Gate"), id("Hauz Khas"), Criterion::Fastest, ctx);
        auto alts = finder.alternatives(id("Kashmere Gate"), id("Hauz Khas"), ctx, 2);
        CHECK(!alts.empty());
        for (const Route& r : alts) CHECK(r.signature() != base->signature());
    });

    run("Recommender ranks options and explains its choice", [&] {
        Recommender rec(finder);
        auto result = rec.recommend(id("Dwarka Sector 21"), id("Noida City Centre"), ctx, Preference::Balanced);
        CHECK(result.options.size() >= 2);
        for (std::size_t i = 1; i < result.options.size(); ++i)
            CHECK(result.options[i - 1].score <= result.options[i].score + 1e-9);
        CHECK(!result.reasons.empty());
        auto hurry = rec.recommend(id("Dwarka Sector 21"), id("Noida City Centre"), ctx, Preference::Hurry);
        double fastest = hurry.options.front().route.totalMin;
        for (const auto& o : hurry.options) CHECK(fastest <= o.route.totalMin + 15); // speed dominates
    });

    run("Journey history persists through save/load", [] {
        const std::string path = "test_history.tmp";
        {
            JourneyHistory h(path, 3);
            for (int i = 0; i < 5; ++i) h.add({"2026-01-01 09:00", "A" + std::to_string(i), "B", "Fastest", 10.0 + i, 20, 0});
        }
        JourneyHistory loaded(path, 3);
        CHECK(loaded.load());
        CHECK(loaded.records().size() == 3);
        CHECK(loaded.records().front().from == "A2");
        CHECK(loaded.records().back().from == "A4");
        std::remove(path.c_str());
    });

    std::cout << "\n  " << (g_checks - g_failed) << "/" << g_checks << " checks passed\n\n";
    return g_failed == 0 ? 0 : 1;
}
