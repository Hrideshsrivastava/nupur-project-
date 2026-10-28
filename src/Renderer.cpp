#include "metro/Renderer.h"

#include "metro/FareCalculator.h"
#include "metro/Sorting.h"
#include "metro/StringUtil.h"
#include "metro/Terminal.h"
#include "metro/UI.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace metro {

using ui::Align;
using ui::muted;
using ui::pad;
using ui::paint;
using ui::repeat;
using ui::strong;
namespace pal = ui::palette;

namespace {

std::string pct(double level) { return std::to_string(static_cast<int>(std::lround(level))) + "%"; }

// Dark or light text, whichever reads better on `bgColor`.
RGB inkFor(RGB bgColor) {
    const double luminance = 0.299 * bgColor.r + 0.587 * bgColor.g + 0.114 * bgColor.b;
    return luminance > 140 ? pal::ink : RGB{250, 250, 255};
}

} // namespace

Renderer::Renderer(const MetroGraph& graph, const CrowdModel& crowd) : graph_(graph), crowd_(crowd) {}

// ─── Building blocks ────────────────────────────────────────────────────────

std::string Renderer::lineChip(int line) const {
    if (line < 0) return muted("walk");
    const MetroLine& l = graph_.line(line);
    if (term::colorMode() == term::ColorMode::None) return "[" + l.code + "]";
    // Fixed width (codes are 2–3 letters) so chips line up in columns.
    return term::bg(l.color) + term::fg(inkFor(l.color)) + term::bold() + " " + pad(l.code, 3) + " " +
           term::reset();
}

std::string Renderer::stationChips(int station) const {
    std::string out;
    for (int l : graph_.station(station).lines) out += lineChip(l) + " ";
    if (!out.empty()) out.pop_back();
    return out;
}

std::string Renderer::routeRibbon(const Route& route, int width) const {
    const auto& segs = route.segments;
    if (segs.empty()) return "";
    const int junctions = static_cast<int>(segs.size()) - 1;
    const int available = std::max(static_cast<int>(segs.size()), width - 2 - junctions);

    double total = 0.0;
    for (const Segment& s : segs) total += std::max(0.5, s.rideMin);

    // Allocate characters proportionally to time spent on each segment.
    std::vector<int> cells;
    int used = 0;
    for (std::size_t i = 0; i < segs.size(); ++i) {
        int c = std::max(1, static_cast<int>(std::lround(available * std::max(0.5, segs[i].rideMin) / total)));
        if (i + 1 == segs.size()) c = std::max(1, available - used);
        cells.push_back(c);
        used += c;
    }

    const RGB first = segs.front().isWalk() ? pal::muted : graph_.line(segs.front().line).color;
    const RGB last = segs.back().isWalk() ? pal::muted : graph_.line(segs.back().line).color;
    std::string out = paint("◉", first, true);
    for (std::size_t i = 0; i < segs.size(); ++i) {
        if (segs[i].isWalk()) out += paint(repeat("┄", static_cast<std::size_t>(cells[i])), pal::muted);
        else out += paint(repeat("━", static_cast<std::size_t>(cells[i])), graph_.line(segs[i].line).color, true);
        if (i + 1 < segs.size()) out += paint("●", pal::text, true);
    }
    return out + paint("◉", last, true);
}

std::string Renderer::directionOf(int line, int a, int b) const {
    const auto& st = graph_.line(line).stations;
    const auto ia = std::find(st.begin(), st.end(), a) - st.begin();
    const auto ib = std::find(st.begin(), st.end(), b) - st.begin();
    return graph_.station(ib > ia ? st.back() : st.front()).name;
}

std::string Renderer::crowdCell(double level, bool withLabel) const {
    std::string out = ui::crowdMeter(level, 8) + " " + paint(pad(pct(level), 4, Align::Right), ui::crowdColor(level), true);
    if (withLabel) out += " " + paint(pad(CrowdModel::label(level), 8), ui::crowdColor(level));
    else out += "         ";
    return out;
}

RGB Renderer::badgeColor(const std::string& b) {
    if (b == "FASTEST") return {0, 210, 255};
    if (b == "CHEAPEST") return {80, 220, 140};
    if (b == "DIRECT" || b == "FEWEST CHANGES") return {170, 110, 255};
    if (b == "LEAST CROWDED") return {255, 205, 60};
    if (b == "SHORTEST") return {255, 140, 90};
    if (b == "FEWEST STOPS") return {255, 95, 170};
    if (b == "RECOMMENDED") return pal::gold;
    return pal::muted;
}

// ─── Banner ─────────────────────────────────────────────────────────────────

void Renderer::banner(bool animated) const {
    static const char* const art[] = {
        "███╗   ███╗ ███████╗ ████████╗ ██████╗   ██████╗ ",
        "████╗ ████║ ██╔════╝ ╚══██╔══╝ ██╔══██╗ ██╔═══██╗",
        "██╔████╔██║ █████╗      ██║    ██████╔╝ ██║   ██║",
        "██║╚██╔╝██║ ██╔══╝      ██║    ██╔══██╗ ██║   ██║",
        "██║ ╚═╝ ██║ ███████╗    ██║    ██║  ██║ ╚██████╔╝",
        "╚═╝     ╚═╝ ╚══════╝    ╚═╝    ╚═╝  ╚═╝  ╚═════╝ ",
    };
    const int w = ui::layoutWidth();
    const int artWidth = static_cast<int>(ui::visibleWidth(art[0]));
    const std::string indent(static_cast<std::size_t>(std::max(2, 2 + (w - artWidth) / 2)), ' ');

    std::cout << "\n";
    for (int row = 0; row < 6; ++row) {
        const RGB from = ui::lerp(pal::accent, pal::accent2, row / 8.0);
        const RGB to = ui::lerp(pal::accent3, pal::gold, row / 6.0);
        std::cout << indent << ui::gradient(art[row], from, to, true) << "\n";
        if (animated) term::sleepMs(45);
    }

    const std::string tagline = "I N T E L L I G E N T   R O U T E   N A V I G A T O R";
    const int tagWidth = static_cast<int>(ui::visibleWidth(tagline));
    std::cout << "\n" << std::string(static_cast<std::size_t>(std::max(2, 2 + (w - tagWidth) / 2)), ' ')
              << ui::gradient(tagline, pal::text, pal::muted) << "\n\n";

    // A stripe in the colours of every line, drawn left to right.
    const int stripe = std::min(w, 72);
    const std::string stripeIndent(static_cast<std::size_t>(2 + (w - stripe) / 2), ' ');
    const int perLine = stripe / static_cast<int>(graph_.lineCount());
    std::cout << stripeIndent;
    for (const MetroLine& l : graph_.lines()) {
        std::cout << paint(repeat("▀", static_cast<std::size_t>(perLine)), l.color);
        if (animated) term::sleepMs(35);
    }
    std::cout << "\n";

    const std::string stats = std::to_string(graph_.lineCount()) + " lines  ·  " +
                              std::to_string(graph_.stationCount()) + " stations  ·  " +
                              std::to_string(graph_.interchangeStations().size()) + " interchanges  ·  " +
                              str::fixed(graph_.totalTrackKm(), 0) + " km of track  ·  Delhi NCR";
    const int statsWidth = static_cast<int>(ui::visibleWidth(stats));
    std::cout << std::string(static_cast<std::size_t>(std::max(2, 2 + (w - statsWidth) / 2)), ' ')
              << muted(stats) << "\n";
}

// ─── Journey card ───────────────────────────────────────────────────────────

void Renderer::journeyCard(const Route& route, const std::string& title, const JourneyContext& ctx,
                           const std::vector<std::string>& badges) const {
    const int w = ui::layoutWidth();
    const int inner = w - 4;
    const std::string from = graph_.station(route.stations.front()).name;
    const std::string to = graph_.station(route.stations.back()).name;
    const RGB fromColor = route.segments.front().isWalk() ? pal::text : graph_.line(route.segments.front().line).color;
    const RGB toColor = route.segments.back().isWalk() ? pal::text : graph_.line(route.segments.back().line).color;

    // Title plus as many badges as fit on the panel's top border.
    std::string head = ui::gradient(title, pal::accent, pal::accent3, true);
    for (const std::string& b : badges) {
        const std::string next = head + " " + ui::badge(b, badgeColor(b));
        if (static_cast<int>(ui::visibleWidth(next)) > w - 8) break;
        head = next;
    }

    auto spread = [&](const std::string& left, const std::string& right) {
        const int gap = std::max(1, inner - static_cast<int>(ui::visibleWidth(left)) - static_cast<int>(ui::visibleWidth(right)));
        return left + std::string(static_cast<std::size_t>(gap), ' ') + right;
    };

    std::vector<std::string> lines;
    lines.push_back("");
    lines.push_back(spread(muted("FROM"), muted("TO")));
    lines.push_back(spread(paint(from, fromColor, true), paint(to, toColor, true)));
    lines.push_back(routeRibbon(route, inner));
    lines.push_back(spread(muted("depart ") + strong(str::formatClock(ctx.departMinute)),
                           muted("arrive ") + strong(str::formatClock(ctx.departMinute + route.totalMin))));
    lines.push_back("");

    // Six stat tiles: value on top, label below.
    struct Tile {
        std::string value, label;
        RGB color;
    };
    const Tile tiles[] = {
        {str::formatDuration(route.totalMin), "DURATION", pal::accent},
        {str::fixed(route.distanceKm, 1) + " km", "DISTANCE", RGB{255, 140, 90}},
        {std::to_string(route.stops()), "STOPS", pal::accent3},
        {std::to_string(route.interchanges), "CHANGES", pal::accent2},
        {"₹" + std::to_string(route.fare), "FARE", pal::success},
        {pct(route.avgCrowd), "AVG CROWD", ui::crowdColor(route.avgCrowd)},
    };
    const std::size_t tileWidth = static_cast<std::size_t>(inner / 6);
    std::string values, labels;
    for (const Tile& t : tiles) {
        values += pad(paint(t.value, t.color, true), tileWidth);
        labels += pad(muted(t.label), tileWidth);
    }
    lines.push_back(values);
    lines.push_back(labels);
    lines.push_back("");

    std::string crowdLine = muted("Crowd  ") + ui::crowdMeter(route.avgCrowd, 18) + "  " +
                            paint(CrowdModel::label(route.avgCrowd), ui::crowdColor(route.avgCrowd), true);
    if (route.peakCrowdStation >= 0)
        crowdLine += muted("   peak ") + paint(pct(route.peakCrowd), ui::crowdColor(route.peakCrowd), true) +
                     muted(" at ") + graph_.station(route.peakCrowdStation).name;
    lines.push_back(crowdLine);
    lines.push_back(muted("Smart card ") + paint("₹" + std::to_string(route.smartCardFare), pal::success, true) +
                    muted("   ·   CO₂ saved vs. car ≈ ") +
                    paint(str::fixed(route.co2SavedKg(), 1) + " kg", pal::success, true) +
                    muted("   ·   ") + muted(dayTypeName(ctx.day)));

    std::cout << "\n";
    ui::panel(head, lines, ui::lerp(pal::border, pal::accent, 0.35), w);
}

// ─── Timeline ───────────────────────────────────────────────────────────────

void Renderer::timeline(const Route& route, const JourneyContext& ctx) const {
    const int w = ui::layoutWidth();
    const std::size_t nameWidth = static_cast<std::size_t>(std::max(18, w - 36));
    const std::string gutter = "         "; // aligns with "  HH:MM  "

    auto stationRow = [&](std::size_t idx, const std::string& glyph, bool major) {
        const int id = route.stations[idx];
        const std::string clock = str::formatClock(ctx.departMinute + route.etaMin[idx]);
        std::string name = ui::truncate(graph_.station(id).name, nameWidth);
        name = major ? strong(name) : muted(name);
        std::cout << "  " << (major ? paint(clock, pal::text, true) : muted(clock)) << "  " << glyph << "  "
                  << pad(name, nameWidth) << crowdCell(route.crowd[idx], major) << "\n";
    };

    ui::heading("JOURNEY TIMELINE", "station-by-station with predicted crowd");
    std::cout << "\n";
    std::size_t cursor = 0;
    for (std::size_t s = 0; s < route.segments.size(); ++s) {
        const Segment& seg = route.segments[s];
        const RGB color = seg.isWalk() ? pal::muted : graph_.line(seg.line).color;

        if (s == 0) stationRow(0, paint("◉", color, true), true);

        if (seg.transferBeforeMin > 0) {
            std::cout << gutter << paint("┊", pal::warning) << "\n";
            std::cout << gutter << paint("┊", pal::warning) << "  " << paint("⇄", pal::warning, true) << "  "
                      << paint("Change to ", pal::warning) << lineChip(seg.line) << " "
                      << paint(graph_.line(seg.line).name, color, true)
                      << muted("  ·  platform change ≈ " + str::formatDuration(seg.transferBeforeMin)) << "\n";
            std::cout << gutter << paint("┊", pal::warning) << "\n";
        }

        if (seg.isWalk()) {
            std::cout << gutter << paint("┊", pal::muted) << "  " << paint("⇢", pal::text, true) << "  "
                      << strong("Walk " + std::to_string(static_cast<int>(std::lround(seg.distanceKm * 1000))) + " m")
                      << muted(" via skywalk  ·  " + str::formatDuration(seg.rideMin)) << "\n";
        } else {
            const MetroLine& l = graph_.line(seg.line);
            std::cout << gutter << paint("┃", color, true) << "  " << lineChip(seg.line) << " "
                      << paint(l.name, color, true) << muted(" towards ")
                      << strong(directionOf(seg.line, seg.stations[0], seg.stations[1]))
                      << muted("  ·  " + std::to_string(seg.stops()) + " stops  ·  " +
                               str::fixed(seg.distanceKm, 1) + " km  ·  " + str::formatDuration(seg.rideMin))
                      << "\n";
        }

        // Intermediate stations (collapsed when there are many).
        const std::size_t startIdx = cursor; // segments are contiguous
        const std::size_t endIdx = startIdx + seg.stations.size() - 1;
        const std::size_t middle = seg.stations.size() >= 2 ? seg.stations.size() - 2 : 0;
        for (std::size_t k = 1; k + 1 < seg.stations.size(); ++k) {
            const bool collapse = middle > 8 && k > 3 && k + 2 < seg.stations.size() - 1;
            if (collapse) {
                if (k == 4)
                    std::cout << gutter << paint("┃", color) << "  "
                              << muted("⋮  " + std::to_string(middle - 5) + " more stops") << "\n";
                continue;
            }
            stationRow(startIdx + k, paint("○", color), false);
        }

        const bool final = s + 1 == route.segments.size();
        stationRow(endIdx, paint(final ? "◉" : "●", color, true), true);
        cursor = endIdx;
    }
    std::cout << "\n  " << paint("⚑", pal::success, true) << "  "
              << strong("Arrive at " + graph_.station(route.stations.back()).name + " by " +
                        str::formatClock(ctx.departMinute + route.totalMin))
              << muted("  ·  total " + str::formatDuration(route.totalMin)) << "\n";
}

void Renderer::reasons(const std::vector<Reason>& reasons) const {
    if (reasons.empty()) return;
    std::cout << "\n  " << ui::gradient("WHY THIS ROUTE?", pal::gold, pal::accent3, true) << "\n";
    for (const Reason& r : reasons) {
        if (r.positive) std::cout << "    " << paint("✔", pal::success, true) << "  " << r.text << "\n";
        else std::cout << "    " << paint("▲", pal::warning, true) << "  " << paint(r.text, pal::warning) << "\n";
    }
}

// ─── Comparison ─────────────────────────────────────────────────────────────

void Renderer::comparison(const std::vector<ScoredRoute>& options) const {
    const bool wide = ui::layoutWidth() >= 96;
    ui::heading("ROUTE COMPARISON", std::to_string(options.size()) + " distinct options ranked by match score");
    std::cout << "\n";

    ui::Table table;
    table.addColumn("#", Align::Right);
    table.addColumn("Route");
    table.addColumn("Time", Align::Right);
    if (wide) table.addColumn("Dist", Align::Right);
    if (wide) table.addColumn("Stops", Align::Right);
    table.addColumn("Chg", Align::Right);
    table.addColumn("Fare", Align::Right);
    table.addColumn("Crowd");
    table.addColumn("Match", Align::Right);

    for (std::size_t i = 0; i < options.size(); ++i) {
        const ScoredRoute& o = options[i];
        const Route& r = o.route;
        std::string via;
        for (std::size_t s = 0; s < r.segments.size(); ++s) {
            if (s == 3) {
                via += muted(" …");
                break;
            }
            if (s > 0) via += muted("›");
            via += lineChip(r.segments[s].line);
        }
        const RGB matchColor = ui::lerp(pal::danger, pal::success, o.match / 100.0);
        std::vector<std::string> row;
        row.push_back(i == 0 ? paint("★1", pal::gold, true) : muted(std::to_string(i + 1)));
        row.push_back(via);
        row.push_back(i == 0 ? strong(str::formatDuration(r.totalMin)) : str::formatDuration(r.totalMin));
        if (wide) row.push_back(str::fixed(r.distanceKm, 1) + " km");
        if (wide) row.push_back(std::to_string(r.stops()));
        row.push_back(std::to_string(r.interchanges));
        row.push_back("₹" + std::to_string(r.fare));
        row.push_back(ui::crowdMeter(r.avgCrowd, 5) + " " + paint(pct(r.avgCrowd), ui::crowdColor(r.avgCrowd)));
        row.push_back(paint(std::to_string(o.match) + "%", matchColor, true));
        table.addRow(row);
    }
    table.print();

    for (std::size_t i = 0; i < options.size(); ++i) {
        if (options[i].badges.empty()) continue;
        std::cout << "    " << (i == 0 ? paint("★1", pal::gold, true) : muted(pad(std::to_string(i + 1), 2, Align::Right)))
                  << "  ";
        for (const std::string& b : options[i].badges) std::cout << ui::badge(b, badgeColor(b)) << " ";
        std::cout << "\n";
    }
}

// ─── Stations ───────────────────────────────────────────────────────────────

void Renderer::stationCard(int station, const JourneyContext& ctx) const {
    const Station& s = graph_.station(station);
    std::vector<std::string> lines;
    lines.push_back("");
    lines.push_back(ui::gradient(s.name, pal::accent, pal::accent3, true) + "   " +
                    (s.isInterchange() ? ui::badge("INTERCHANGE", pal::accent2) : ui::badge("STATION", pal::border, pal::text)));
    lines.push_back("");

    std::string served = muted("Lines      ");
    for (int l : s.lines) served += lineChip(l) + " " + paint(graph_.line(l).name, graph_.line(l).color, true) + "   ";
    lines.push_back(served);

    const int stars = std::clamp((s.footfall + 1) / 2, 1, 5);
    lines.push_back(muted("Footfall   ") + paint(repeat("★", static_cast<std::size_t>(stars)), pal::gold) +
                    paint(repeat("☆", static_cast<std::size_t>(5 - stars)), pal::border) +
                    muted("  (" + std::to_string(s.footfall) + "/10 popularity)"));

    const double now = crowd_.level(station, ctx.departMinute, ctx.day);
    lines.push_back(muted("Crowd now  ") + ui::crowdMeter(now, 24) + "  " +
                    paint(pct(now) + " " + CrowdModel::label(now), ui::crowdColor(now), true) +
                    muted("  at " + str::formatClock(ctx.departMinute)));
    lines.push_back("");

    // Neighbours grouped by line.
    lines.push_back(muted("CONNECTIONS"));
    std::vector<int> seenLines;
    for (const Edge& e : graph_.neighbors(station)) {
        if (std::find(seenLines.begin(), seenLines.end(), e.line) != seenLines.end()) continue;
        seenLines.push_back(e.line);
        std::string row = "  " + lineChip(e.line) + "  ";
        bool first = true;
        for (const Edge& n : graph_.neighbors(station)) {
            if (n.line != e.line) continue;
            if (!first) row += muted("  ·  ");
            first = false;
            row += graph_.station(n.to).name + muted(" (" + str::fixed(n.distanceKm, 1) + " km, " +
                                                    str::formatDuration(n.timeMin) + ")");
        }
        lines.push_back(row);
    }
    lines.push_back("");
    std::cout << "\n";
    ui::panel(ui::gradient("STATION", pal::accent, pal::accent2, true), lines);
}

void Renderer::crowdForecast(int station, const JourneyContext& ctx) const {
    const std::vector<double> values = crowd_.forecast(station, ctx.day);
    constexpr int height = 8;
    static const char* const lower[] = {" ", "▁", "▂", "▃", "▄", "▅", "▆", "▇", "█"};
    const int nowHour = (ctx.departMinute / 60) % 24;

    ui::heading("24-HOUR CROWD FORECAST", std::string(dayTypeName(ctx.day)) + " · " + graph_.station(station).name);
    std::cout << "\n";
    for (int row = height - 1; row >= 0; --row) {
        std::string axis = row == height - 1 ? "100%" : row == height / 2 ? " 50%" : row == 0 ? "  0%" : "    ";
        std::cout << "  " << muted(axis) << " " << paint(row == 0 ? "┼" : "┤", pal::border);
        for (int h = 0; h < 24; ++h) {
            const double v = values[static_cast<std::size_t>(h)] / 100.0 * height;
            const double fill = std::clamp(v - row, 0.0, 1.0);
            const int eighths = static_cast<int>(std::lround(fill * 8));
            const RGB c = ui::crowdColor(values[static_cast<std::size_t>(h)]);
            const std::string cell = std::string(lower[eighths]) + lower[eighths];
            std::cout << (eighths > 0 ? paint(cell, h == nowHour ? ui::lerp(c, RGB{255, 255, 255}, 0.35) : c) : "  ") << " ";
        }
        std::cout << "\n";
    }
    std::cout << "        ";
    for (int h = 0; h < 24; ++h) std::cout << (h % 3 == 0 ? muted(pad(str::formatClock(h * 60).substr(0, 2), 3)) : "   ");
    std::cout << "\n        " << std::string(static_cast<std::size_t>(nowHour * 3), ' ') << paint("▲ now", pal::gold, true) << "\n";

    const auto peak = std::max_element(values.begin(), values.end());
    const auto calm = std::min_element(values.begin() + 6, values.begin() + 23); // daytime only
    std::cout << "\n  " << muted("Busiest around ") << paint(str::formatClock((peak - values.begin()) * 60) , pal::danger, true)
              << muted(" (" + pct(*peak) + ")   ·   calmest daytime slot ")
              << paint(str::formatClock((calm - values.begin()) * 60), pal::success, true) << muted(" (" + pct(*calm) + ")")
              << "\n";
}

// ─── Lines ──────────────────────────────────────────────────────────────────

void Renderer::linesOverview() const {
    std::vector<int> order;
    for (std::size_t i = 0; i < graph_.lineCount(); ++i) order.push_back(static_cast<int>(i));
    mergeSort(order, [&](int a, int b) { return graph_.line(a).lengthKm > graph_.line(b).lengthKm; });

    double longest = 0;
    for (const MetroLine& l : graph_.lines()) longest = std::max(longest, l.lengthKm);

    ui::heading("METRO LINES", "sorted by length (merge sort)");
    std::cout << "\n";
    ui::Table t;
    t.addColumn("Line");
    t.addColumn("Name");
    t.addColumn("Stations", Align::Right);
    t.addColumn("Length", Align::Right);
    t.addColumn("");
    t.addColumn("End to end", Align::Right);
    for (int idx : order) {
        const MetroLine& l = graph_.line(idx);
        double minutes = 0;
        for (std::size_t i = 0; i + 1 < l.stations.size(); ++i)
            minutes += graph_.edgeBetween(l.stations[i], l.stations[i + 1], idx)->timeMin;
        t.addRow({lineChip(idx), paint(l.name, l.color, true), std::to_string(l.stations.size()),
                  str::fixed(l.lengthKm, 1) + " km", ui::meter(l.lengthKm / longest, 16, ui::lerp(l.color, RGB{20, 20, 30}, 0.4), l.color),
                  str::formatDuration(minutes)});
    }
    t.print();
}

void Renderer::lineDiagram(int line) const {
    const MetroLine& l = graph_.line(line);
    const int w = ui::layoutWidth();
    double minutes = 0;
    for (std::size_t i = 0; i + 1 < l.stations.size(); ++i)
        minutes += graph_.edgeBetween(l.stations[i], l.stations[i + 1], line)->timeMin;
    int interchanges = 0;
    for (int id : l.stations) interchanges += graph_.station(id).isInterchange() ? 1 : 0;

    std::vector<std::string> head;
    head.push_back("");
    head.push_back(lineChip(line) + "  " + paint(l.name, l.color, true) +
                   (l.premium ? "  " + ui::badge("PREMIUM FARE", pal::gold) : ""));
    head.push_back(paint(repeat("━", static_cast<std::size_t>(w - 4)), l.color, true));
    head.push_back(muted("Route    ") + strong(graph_.station(l.stations.front()).name) + muted("  ⇄  ") +
                   strong(graph_.station(l.stations.back()).name));
    head.push_back(muted("Stations ") + strong(std::to_string(l.stations.size())) + muted("   ·   Length ") +
                   strong(str::fixed(l.lengthKm, 1) + " km") + muted("   ·   End to end ") +
                   strong(str::formatDuration(minutes)) + muted("   ·   Interchanges ") +
                   strong(std::to_string(interchanges)));
    head.push_back("");
    std::cout << "\n";
    ui::panel(ui::gradient("LINE DIAGRAM", pal::accent, pal::accent2, true), head, l.color, w);
    std::cout << "\n";

    // One entry per station: track glyph, name, other lines, km from origin.
    const bool twoColumns = w >= 96;
    const std::size_t colWidth = twoColumns ? static_cast<std::size_t>((w - 4) / 2) : static_cast<std::size_t>(w - 2);
    std::vector<std::string> entries;
    double km = 0.0;
    for (std::size_t i = 0; i < l.stations.size(); ++i) {
        const int id = l.stations[i];
        if (i > 0) km += graph_.edgeBetween(l.stations[i - 1], id, line)->distanceKm;
        const Station& s = graph_.station(id);
        std::string others;
        for (int other : s.lines)
            if (other != line) others += " " + lineChip(other);
        if (s.hasWalkLink) others += " " + muted("⇢walk");
        const bool terminus = i == 0 || i + 1 == l.stations.size();
        const std::string glyph = paint(s.isInterchange() ? "◉" : (terminus ? "■" : "●"), l.color, true);
        const std::size_t othersWidth = ui::visibleWidth(others);
        const std::size_t nameRoom = colWidth > othersWidth + 16 ? colWidth - othersWidth - 16 : 8;
        std::string name = ui::truncate(s.name, nameRoom);
        name = s.isInterchange() || terminus ? strong(name) : paint(name, pal::text);
        std::string entry = paint(i == 0 ? "╻" : "┃", l.color) + glyph + " " + name + others;
        entry = pad(entry, colWidth - 8) + muted(pad(str::fixed(km, 1) + " km", 8, Align::Right));
        entries.push_back(entry);
    }

    if (twoColumns) {
        const std::size_t half = (entries.size() + 1) / 2;
        for (std::size_t i = 0; i < half; ++i) {
            std::cout << "  " << pad(entries[i], colWidth);
            if (i + half < entries.size()) std::cout << "  " << entries[i + half];
            std::cout << "\n";
        }
    } else {
        for (const std::string& e : entries) std::cout << "  " << e << "\n";
    }
    std::cout << "\n  " << muted("◉ interchange   ■ terminus   ● station   ⇢walk skywalk transfer") << "\n";
}

// ─── Crowd heatmap ──────────────────────────────────────────────────────────

void Renderer::heatmap(const JourneyContext& ctx, int topN) const {
    struct Row {
        int id;
        double level;
    };
    std::vector<Row> rows;
    double sum = 0;
    int packed = 0;
    for (const Station& s : graph_.stations()) {
        const double lvl = crowd_.level(s.id, ctx.departMinute, ctx.day);
        rows.push_back({s.id, lvl});
        sum += lvl;
        if (lvl >= 75) ++packed;
    }
    mergeSort(rows, [](const Row& a, const Row& b) { return a.level > b.level; });

    const double avg = sum / static_cast<double>(rows.size());
    ui::heading("LIVE CROWD HEATMAP", str::formatClock(ctx.departMinute) + " · " + dayTypeName(ctx.day));
    std::cout << "\n  " << muted("Network load ") << ui::crowdMeter(avg, 30) << "  "
              << paint(pct(avg) + " " + CrowdModel::label(avg), ui::crowdColor(avg), true) << muted("   ·   ")
              << paint(std::to_string(packed), pal::danger, true) << muted(" stations packed   ·   ")
              << (CrowdModel::isPeak(ctx.departMinute, ctx.day) ? ui::badge("PEAK HOURS", pal::danger)
                                                                : ui::badge("OFF-PEAK", pal::success))
              << "\n\n";

    const int w = ui::layoutWidth();
    const std::size_t nameWidth = static_cast<std::size_t>(std::max(20, w - 62));
    for (int i = 0; i < topN && i < static_cast<int>(rows.size()); ++i) {
        const Row& r = rows[static_cast<std::size_t>(i)];
        std::cout << "  " << muted(pad(std::to_string(i + 1), 3, Align::Right)) << "  "
                  << pad(strong(ui::truncate(graph_.station(r.id).name, nameWidth)), nameWidth) << "  "
                  << pad(stationChips(r.id), 14) << "  " << ui::crowdMeter(r.level, 26) << " "
                  << paint(pad(pct(r.level), 4, Align::Right), ui::crowdColor(r.level), true) << " "
                  << paint(CrowdModel::label(r.level), ui::crowdColor(r.level)) << "\n";
    }
    const Row& calm = rows.back();
    std::cout << "\n  " << muted("Quietest right now: ") << strong(graph_.station(calm.id).name)
              << muted(" (" + pct(calm.level) + ")") << "\n";
}

// ─── Network statistics ─────────────────────────────────────────────────────

void Renderer::networkStats(const Trie& trie) const {
    // Graph diameter in hops: BFS from every station.
    int diameter = 0, endA = 0, endB = 0;
    for (const Station& s : graph_.stations()) {
        const std::vector<int> hops = graph_.bfsHops(s.id);
        for (std::size_t j = 0; j < hops.size(); ++j)
            if (hops[j] > diameter) {
                diameter = hops[j];
                endA = s.id;
                endB = static_cast<int>(j);
            }
    }
    int skywalks = 0;
    std::size_t degreeSum = 0;
    for (const Station& s : graph_.stations()) {
        for (const Edge& e : graph_.neighbors(s.id)) skywalks += e.isWalk() ? 1 : 0;
        degreeSum += graph_.neighbors(s.id).size();
    }
    skywalks /= 2;
    const bool connected = graph_.isConnected();

    auto stat = [](const std::string& label, const std::string& value, RGB color) {
        return pad(muted(label), 26) + paint(value, color, true);
    };
    std::vector<std::string> lines;
    lines.push_back("");
    lines.push_back(stat("Stations (vertices)", std::to_string(graph_.stationCount()), pal::accent));
    lines.push_back(stat("Connections (edges)", std::to_string(graph_.connectionCount()), pal::accent));
    lines.push_back(stat("Metro lines", std::to_string(graph_.lineCount()), pal::accent2));
    lines.push_back(stat("Interchange stations", std::to_string(graph_.interchangeStations().size()), pal::accent2));
    lines.push_back(stat("Skywalk transfers", std::to_string(skywalks), pal::accent2));
    lines.push_back(stat("Total track length", str::fixed(graph_.totalTrackKm(), 1) + " km", pal::accent3));
    lines.push_back(stat("Avg station spacing", str::fixed(graph_.totalTrackKm() / static_cast<double>(graph_.connectionCount() - static_cast<std::size_t>(skywalks)), 2) + " km", pal::accent3));
    lines.push_back(stat("Average degree", str::fixed(static_cast<double>(degreeSum) / static_cast<double>(graph_.stationCount()), 2), pal::accent3));
    lines.push_back(stat("Connectivity (BFS)", connected ? "✔ every station reachable" : "✖ disconnected!",
                         connected ? pal::success : pal::danger));
    lines.push_back(stat("Network diameter", std::to_string(diameter) + " stops", pal::gold));
    lines.push_back(pad("", 26) + muted(graph_.station(endA).name + "  ⇄  " + graph_.station(endB).name));
    lines.push_back(stat("Trie index", std::to_string(trie.nodeCount()) + " nodes · " + std::to_string(trie.wordCount()) + " names", pal::success));
    lines.push_back("");
    std::cout << "\n";
    ui::panel(ui::gradient("NETWORK STATISTICS", pal::accent, pal::accent2, true), lines);

    // Biggest hubs by number of lines, then popularity (merge sort).
    std::vector<int> hubs = graph_.interchangeStations();
    mergeSort(hubs, [&](int a, int b) {
        const Station& x = graph_.station(a);
        const Station& y = graph_.station(b);
        if (x.lines.size() != y.lines.size()) return x.lines.size() > y.lines.size();
        return x.footfall > y.footfall;
    });
    std::cout << "\n  " << ui::gradient("TOP INTERCHANGE HUBS", pal::gold, pal::accent3, true) << "\n\n";
    for (std::size_t i = 0; i < hubs.size() && i < 10; ++i) {
        const Station& s = graph_.station(hubs[i]);
        std::cout << "    " << muted(pad(std::to_string(i + 1), 2, Align::Right)) << "  "
                  << pad(strong(s.name), 34) << stationChips(s.id) << "\n";
    }
}

void Renderer::fareChart() const {
    ui::heading("FARE CHART", "distance-based slabs");
    std::cout << "\n";
    ui::Table t;
    t.addColumn("Distance");
    t.addColumn("Mon–Sat", Align::Right);
    t.addColumn("Sunday", Align::Right);
    t.addColumn("Smart card", Align::Right);
    double previous = 0;
    for (const auto& s : FareCalculator::slabs()) {
        const std::string range = s.upToKm > 1e8 ? "above " + str::fixed(previous, 0) + " km"
                                                 : str::fixed(previous, 0) + " – " + str::fixed(s.upToKm, 0) + " km";
        t.addRow({range, paint("₹" + std::to_string(s.weekday), pal::success, true), "₹" + std::to_string(s.sunday),
                  "₹" + std::to_string(FareCalculator::smartCardFare(s.weekday))});
        previous = s.upToKm;
    }
    t.print();
    std::cout << "\n  " << lineChip(graph_.findLine("AE")) << muted("  Airport Express is charged separately: ₹20 + ₹1.8/km (max ₹75)") << "\n";
}

void Renderer::history(const JourneyHistory& history) const {
    const auto& q = history.records();
    ui::heading("JOURNEY HISTORY", "last " + std::to_string(q.capacity()) + " journeys · FIFO queue");
    std::cout << "\n";
    if (q.empty()) {
        ui::info("No journeys yet — plan one from the main menu.");
        return;
    }
    ui::Table t;
    t.addColumn("#", Align::Right);
    t.addColumn("When");
    t.addColumn("From");
    t.addColumn("");
    t.addColumn("To");
    t.addColumn("Mode");
    t.addColumn("Time", Align::Right);
    t.addColumn("Fare", Align::Right);
    for (std::size_t i = q.size(); i-- > 0;) { // newest first
        const JourneyRecord& r = q.at(i);
        t.addRow({muted(std::to_string(q.size() - i)), muted(r.timestamp), strong(ui::truncate(r.from, 22)),
                  paint("→", pal::accent), strong(ui::truncate(r.to, 22)), r.mode, str::formatDuration(r.minutes),
                  "₹" + std::to_string(r.fare)});
    }
    t.print();
    std::cout << "  " << muted("Saved to " + history.path()) << "\n";
}

} // namespace metro
