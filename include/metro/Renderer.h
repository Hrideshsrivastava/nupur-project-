#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Renderer.h — turns graph data and computed routes into rich terminal
//  visuals: the banner, journey cards, timelines, comparison tables, crowd
//  charts, line diagrams and network statistics.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/CrowdModel.h"
#include "metro/JourneyHistory.h"
#include "metro/MetroGraph.h"
#include "metro/Recommender.h"
#include "metro/Trie.h"

#include <string>
#include <vector>

namespace metro {

class Renderer {
public:
    Renderer(const MetroGraph& graph, const CrowdModel& crowd);

    // ── Small building blocks ───────────────────────────────────────────────
    /// " BL " on the line's colour.
    std::string lineChip(int line) const;
    /// Chips of every line serving a station.
    std::string stationChips(int station) const;
    /// A bar coloured segment-by-segment with each line's colour.
    std::string routeRibbon(const Route& route, int width) const;

    // ── Screens ─────────────────────────────────────────────────────────────
    void banner(bool animated) const;
    void journeyCard(const Route& route, const std::string& title, const JourneyContext& ctx,
                     const std::vector<std::string>& badges) const;
    void timeline(const Route& route, const JourneyContext& ctx) const;
    void reasons(const std::vector<Reason>& reasons) const;
    void comparison(const std::vector<ScoredRoute>& options) const;
    void stationCard(int station, const JourneyContext& ctx) const;
    void crowdForecast(int station, const JourneyContext& ctx) const;
    void linesOverview() const;
    void lineDiagram(int line) const;
    void heatmap(const JourneyContext& ctx, int topN) const;
    void networkStats(const Trie& trie) const;
    void fareChart() const;
    void history(const JourneyHistory& history) const;

    /// Colour used for a badge name such as "FASTEST".
    static RGB badgeColor(const std::string& badge);

private:
    /// Terminus the train is heading to when travelling from a to b on `line`.
    std::string directionOf(int line, int a, int b) const;
    std::string crowdCell(double level, bool withLabel) const;

    const MetroGraph& graph_;
    const CrowdModel& crowd_;
};

} // namespace metro
