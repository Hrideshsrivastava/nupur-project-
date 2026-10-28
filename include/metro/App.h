#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  App.h — the interactive application: owns every subsystem and drives the
//  menus, prompts and screens.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/CrowdModel.h"
#include "metro/JourneyHistory.h"
#include "metro/MetroGraph.h"
#include "metro/Recommender.h"
#include "metro/Renderer.h"
#include "metro/RouteFinder.h"
#include "metro/Trie.h"

#include <optional>
#include <string>

namespace metro {

struct Settings {
    DayType day = DayType::Weekday;
    bool animations = true;
    std::string historyFile = "metro_history.txt";
};

class MetroApp {
public:
    explicit MetroApp(Settings settings);

    /// Full interactive experience with the main menu. Returns an exit code.
    int run();

    /// One-shot mode: print the recommendation for a journey and exit.
    int runQuick(const std::string& from, const std::string& to, const std::string& time,
                 const std::string& preference);

private:
    // Screens reachable from the main menu.
    void planJourney();
    void routeByCriterion();
    void stationExplorer();
    void linesExplorer();
    void crowdHeatmap();
    void showHistory();
    void networkInfo();
    void settingsMenu();

    // Input helpers.
    std::optional<int> askStation(const std::string& label, int exclude = -1);
    std::optional<int> askStationInteractive(const std::string& label, int exclude);
    std::optional<int> askStationPlain(const std::string& label, int exclude);
    std::optional<int> askDeparture();
    std::optional<Preference> askPreference();

    /// Station id for free text: exact hash lookup, then Trie, then edit distance.
    int resolveStation(const std::string& text, std::string* suggestion) const;
    /// Closest station names by Levenshtein distance ("did you mean…").
    std::vector<int> fuzzyMatches(const std::string& text, std::size_t limit) const;
    /// Edit distance between a query and a station name (also tried against
    /// the name's prefix, so partial input is not penalised for its length).
    static std::size_t fuzzyDistance(const std::string& query, const std::string& name);
    /// Stations whose words start with the query's words, in order:
    /// "noida sec 62" -> "Noida Sector 62".
    std::vector<int> wordPrefixMatches(const std::string& text) const;

    void showRecommendation(const Recommendation& rec, const JourneyContext& ctx, Preference pref);
    void afterJourney(const Recommendation& rec, const JourneyContext& ctx);
    void record(const Route& route, const std::string& mode);

    static int currentMinute();

    Settings settings_;
    MetroGraph graph_;
    Trie trie_;
    CrowdModel crowd_;
    RouteFinder finder_;
    Recommender recommender_;
    Renderer renderer_;
    JourneyHistory history_;
};

} // namespace metro
