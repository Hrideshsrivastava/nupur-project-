#include "metro/App.h"

#include "metro/Exporter.h"
#include "metro/MetroData.h"
#include "metro/Sorting.h"
#include "metro/StringUtil.h"
#include "metro/Terminal.h"
#include "metro/UI.h"

#include <algorithm>
#include <cmath>
#include <ctime>
#include <iostream>

namespace metro {

using ui::muted;
using ui::paint;
using ui::strong;
namespace pal = ui::palette;

MetroApp::MetroApp(Settings settings)
    : settings_(std::move(settings)),
      graph_(data::buildDelhiMetro()),
      crowd_(graph_),
      finder_(graph_, crowd_),
      recommender_(finder_),
      renderer_(graph_, crowd_),
      history_(settings_.historyFile) {
    for (const Station& s : graph_.stations()) trie_.insert(s.name, s.id);
    history_.load();
}

int MetroApp::currentMinute() {
    std::time_t t = std::time(nullptr);
    std::tm tm{};
#ifdef _WIN32
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    return tm.tm_hour * 60 + tm.tm_min;
}

// ─── Main loop ──────────────────────────────────────────────────────────────

int MetroApp::run() {
    term::clearScreen();
    renderer_.banner(settings_.animations);

    const std::vector<ui::MenuItem> items = {
        {"Plan a Journey", "smart recommendation + comparison of every route", pal::accent},
        {"Route by Criterion", "fastest · shortest · cheapest · least changes · …", RGB{0, 188, 212}},
        {"Station Explorer", "Trie autocomplete · connections · crowd forecast", pal::accent2},
        {"Metro Lines", "line diagrams for all 12 lines", RGB{240, 98, 146}},
        {"Live Crowd Heatmap", "busiest stations at any time of day", pal::warning},
        {"Journey History", "your recent trips (queue)", pal::success},
        {"Network & Fares", "graph statistics and fare chart", RGB{255, 140, 90}},
        {"Settings", std::string("day: ") + dayTypeName(settings_.day) + " · animations: " +
                         (settings_.animations ? "on" : "off"), pal::muted},
        {"Exit", "see you on the next ride", pal::danger},
    };

    int last = 0;
    while (true) {
        auto menuItems = items;
        menuItems[7].hint = std::string("day: ") + dayTypeName(settings_.day) +
                            " · animations: " + (settings_.animations ? "on" : "off");
        const int choice = ui::menu("MAIN MENU", menuItems, last);
        if (choice < 0 || choice == 8) break;
        last = choice;
        switch (choice) {
            case 0: planJourney(); break;
            case 1: routeByCriterion(); break;
            case 2: stationExplorer(); break;
            case 3: linesExplorer(); break;
            case 4: crowdHeatmap(); break;
            case 5: showHistory(); break;
            case 6: networkInfo(); break;
            case 7: settingsMenu(); break;
            default: break;
        }
        if (!std::cin.good() && !term::interactive()) break; // input exhausted
        ui::pressEnter();
        term::clearScreen();
        renderer_.banner(false);
    }
    std::cout << "\n  " << ui::gradient("Thank you for riding with Metro Navigator. Goodbye!", pal::accent, pal::accent3, true)
              << "\n\n";
    return 0;
}

int MetroApp::runQuick(const std::string& from, const std::string& to, const std::string& time,
                       const std::string& preference) {
    renderer_.banner(false);
    std::string hint;
    const int src = resolveStation(from, &hint);
    if (src < 0) {
        ui::error("Unknown station \"" + from + "\"" + (hint.empty() ? "" : " — did you mean " + hint + "?"));
        return 2;
    }
    const int dst = resolveStation(to, &hint);
    if (dst < 0) {
        ui::error("Unknown station \"" + to + "\"" + (hint.empty() ? "" : " — did you mean " + hint + "?"));
        return 2;
    }
    if (src == dst) {
        ui::error("Source and destination are the same station.");
        return 2;
    }
    JourneyContext ctx{currentMinute(), settings_.day};
    if (!time.empty() && time != "now" && !str::parseClock(time, ctx.departMinute)) {
        ui::error("Invalid time \"" + time + "\" — use HH:MM, e.g. 09:30");
        return 2;
    }
    Preference pref = Preference::Balanced;
    const std::string p = str::toLower(preference);
    if (p == "fast" || p == "hurry") pref = Preference::Hurry;
    else if (p == "budget" || p == "cheap") pref = Preference::Budget;
    else if (p == "comfort") pref = Preference::Comfort;
    else if (p == "simple") pref = Preference::Simple;

    const Recommendation rec = recommender_.recommend(src, dst, ctx, pref);
    if (rec.options.empty()) {
        ui::error("No route found.");
        return 1;
    }
    showRecommendation(rec, ctx, pref);
    std::cout << "\n";
    return 0;
}

// ─── Station input ──────────────────────────────────────────────────────────

std::size_t MetroApp::fuzzyDistance(const std::string& query, const std::string& name) {
    std::size_t d = str::levenshtein(query, name);
    if (name.size() > query.size()) d = std::min(d, str::levenshtein(query, name.substr(0, query.size())) + 1);
    return d;
}

std::vector<int> MetroApp::fuzzyMatches(const std::string& text, std::size_t limit) const {
    const std::string query = str::normalizeWords(text);
    struct Scored {
        int id;
        std::size_t distance;
    };
    std::vector<Scored> scored;
    for (const Station& s : graph_.stations())
        scored.push_back({s.id, fuzzyDistance(query, str::normalizeWords(s.name))});
    mergeSort(scored, [](const Scored& a, const Scored& b) { return a.distance < b.distance; });
    std::vector<int> ids;
    for (std::size_t i = 0; i < scored.size() && ids.size() < limit; ++i)
        if (scored[i].distance <= std::max<std::size_t>(2, query.size() / 2)) ids.push_back(scored[i].id);
    return ids;
}

std::vector<int> MetroApp::wordPrefixMatches(const std::string& text) const {
    const std::vector<std::string> query = str::split(str::trim(str::normalizeWords(text)), ' ');
    std::vector<int> ids;
    if (query.empty() || query.front().empty()) return ids;
    for (const Station& s : graph_.stations()) {
        const std::vector<std::string> words = str::split(str::normalizeWords(s.name), ' ');
        // Greedily match each query word to the next name word it prefixes.
        std::size_t w = 0;
        bool ok = true;
        for (const std::string& q : query) {
            while (w < words.size() && words[w].compare(0, q.size(), q) != 0) ++w;
            if (w == words.size()) {
                ok = false;
                break;
            }
            ++w;
        }
        if (ok) ids.push_back(s.id);
    }
    return ids;
}

int MetroApp::resolveStation(const std::string& text, std::string* suggestion) const {
    if (suggestion) suggestion->clear();
    if (str::trim(text).empty()) return -1;
    const int exact = graph_.findStation(text); // O(1) hash-map lookup
    if (exact >= 0) return exact;
    const std::vector<int> prefix = trie_.suggest(text, 2);
    if (prefix.size() == 1) return prefix.front(); // unambiguous prefix
    const std::vector<int> words = wordPrefixMatches(text);
    if (prefix.empty() && words.size() == 1) return words.front(); // "noida sec 62"

    // Accept a small typo when one station is clearly the closest.
    const std::string query = str::normalizeWords(text);
    const std::vector<int> fuzzy = fuzzyMatches(text, 2);
    if (prefix.empty() && !fuzzy.empty()) {
        const std::size_t best = fuzzyDistance(query, str::normalizeWords(graph_.station(fuzzy[0]).name));
        const std::size_t second = fuzzy.size() > 1
                                       ? fuzzyDistance(query, str::normalizeWords(graph_.station(fuzzy[1]).name))
                                       : best + 1;
        if (best <= 2 && second > best && query.size() >= 4) return fuzzy[0];
    }
    if (suggestion) {
        if (!prefix.empty()) *suggestion = graph_.station(prefix.front()).name;
        else if (!words.empty()) *suggestion = graph_.station(words.front()).name;
        else if (!fuzzy.empty()) *suggestion = graph_.station(fuzzy.front()).name;
    }
    return -1;
}

std::optional<int> MetroApp::askStation(const std::string& label, int exclude) {
    return term::interactive() ? askStationInteractive(label, exclude) : askStationPlain(label, exclude);
}

std::optional<int> MetroApp::askStationPlain(const std::string& label, int exclude) {
    while (true) {
        auto in = ui::prompt(label);
        if (!in || in->empty()) return std::nullopt;
        std::string hint;
        const int id = resolveStation(*in, &hint);
        if (id >= 0 && id != exclude) {
            ui::success(paint(graph_.station(id).name, pal::gold, true) + "  " + renderer_.stationChips(id));
            return id;
        }
        if (id == exclude && id >= 0) ui::error("Pick a different station from the origin.");
        else ui::error("No station named \"" + *in + "\"" + (hint.empty() ? "." : " — did you mean " + hint + "?"));
    }
}

std::optional<int> MetroApp::askStationInteractive(const std::string& label, int exclude) {
    constexpr std::size_t kShown = 6;
    std::string input;
    std::size_t selected = 0;
    std::string status;
    const std::string promptText = "  " + paint("›", pal::accent, true) + " " + strong(label) + " ";

    auto suggestions = [&]() {
        std::vector<int> ids = trie_.suggest(input, kShown + 1);
        ids.erase(std::remove(ids.begin(), ids.end(), exclude), ids.end());
        if (ids.size() > kShown) ids.resize(kShown);
        if (ids.empty()) { // abbreviations ("noida sec 6"), then typo rescue
            ids = wordPrefixMatches(input);
            if (ids.size() > kShown) ids.resize(kShown);
        }
        if (ids.empty() && str::trim(input).size() >= 3) ids = fuzzyMatches(input, 3);
        return ids;
    };

    term::RawMode raw;
    std::vector<int> ids = suggestions();
    auto render = [&]() {
        std::vector<std::string> lines;
        lines.push_back(promptText + paint(input, pal::gold, true) + paint("▏", pal::accent));
        for (std::size_t i = 0; i < kShown; ++i) {
            if (i >= ids.size()) {
                lines.push_back(i == 0 && input.empty() ? "      " + muted("start typing — e.g. \"rajiv\", \"sector 62\", \"airport\"") : "");
                continue;
            }
            const Station& s = graph_.station(ids[i]);
            const std::string chips = renderer_.stationChips(s.id);
            if (i == selected)
                lines.push_back("    " + paint("❯ ", pal::accent, true) + term::bg(RGB{34, 40, 64}) + term::bold() +
                                term::fg(pal::gold) + " " + ui::pad(s.name, 36) + " " + term::reset() + " " + chips);
            else
                lines.push_back("      " + paint(ui::pad(s.name, 38), pal::text) + " " + chips);
        }
        lines.push_back(status.empty() ? "      " + muted("↑/↓ choose   Tab complete   Enter confirm   Esc cancel")
                                       : "      " + status);
        ui::drawBlock(lines);
        return lines.size();
    };

    term::hideCursor();
    std::size_t height = render();
    std::optional<int> result;
    bool done = false;
    while (!done) {
        const int key = term::readKey();
        status.clear();
        if (key == term::KeyEscape || key == term::KeyCtrlC || key == term::KeyEOF) {
            done = true;
        } else if (key == term::KeyUp) {
            if (!ids.empty()) selected = (selected + ids.size() - 1) % ids.size();
        } else if (key == term::KeyDown) {
            if (!ids.empty()) selected = (selected + 1) % ids.size();
        } else if (key == term::KeyTab) {
            if (!ids.empty()) input = graph_.station(ids[selected]).name;
        } else if (key == term::KeyBackspace) {
            if (!input.empty()) input.pop_back();
            selected = 0;
        } else if (key == term::KeyEnter) {
            int id = graph_.findStation(input);
            if (!ids.empty()) id = ids[selected];
            if (id >= 0 && id != exclude) {
                result = id;
                done = true;
            } else if (id == exclude && id >= 0) {
                status = paint("✖ Choose a station different from the origin", pal::danger);
            } else {
                status = paint("✖ No station matches \"" + input + "\"", pal::danger);
            }
        } else if (key >= 32 && key < 127 && input.size() < 40) {
            input.push_back(static_cast<char>(key));
            selected = 0;
        }
        if (!done) {
            ids = suggestions();
            if (selected >= ids.size()) selected = 0;
            height = render();
        }
    }

    // Collapse the widget into a single confirmation line.
    std::vector<std::string> final(height, "");
    final[0] = promptText + (result ? paint(graph_.station(*result).name, pal::gold, true) + "  " +
                                          renderer_.stationChips(*result)
                                    : muted("cancelled"));
    ui::drawBlock(final);
    std::cout << "\n" << std::flush;
    std::cout << "\x1b[J" << std::flush; // clear the now-empty suggestion area
    term::showCursor();
    return result;
}

std::optional<int> MetroApp::askDeparture() {
    const int now = currentMinute();
    const std::vector<ui::MenuItem> items = {
        {"Leave now", str::formatClock(now), pal::accent},
        {"Morning peak", "09:00", pal::danger},
        {"Afternoon", "14:00", pal::success},
        {"Evening peak", "18:00", pal::warning},
        {"Late night", "22:30", pal::accent2},
        {"Custom time…", "enter HH:MM", pal::muted},
    };
    const int choice = ui::menu("WHEN DO YOU WANT TO TRAVEL?", items);
    switch (choice) {
        case 0: return now;
        case 1: return 9 * 60;
        case 2: return 14 * 60;
        case 3: return 18 * 60;
        case 4: return 22 * 60 + 30;
        case 5:
            while (true) {
                auto in = ui::prompt("Departure time (HH:MM):");
                if (!in || in->empty()) return std::nullopt;
                int minute = 0;
                if (str::parseClock(*in, minute)) return minute;
                ui::error("Invalid time — try something like 08:45 or 1730.");
            }
        default: return std::nullopt;
    }
}

std::optional<Preference> MetroApp::askPreference() {
    const Preference prefs[] = {Preference::Balanced, Preference::Hurry, Preference::Budget,
                                Preference::Comfort, Preference::Simple};
    const RGB colors[] = {pal::gold, pal::accent, pal::success, pal::warning, pal::accent2};
    std::vector<ui::MenuItem> items;
    for (int i = 0; i < 5; ++i) items.push_back({preferenceName(prefs[i]), preferenceTagline(prefs[i]), colors[i]});
    const int choice = ui::menu("WHAT MATTERS MOST TO YOU?", items);
    if (choice < 0) return std::nullopt;
    return prefs[choice];
}

// ─── Screens ────────────────────────────────────────────────────────────────

void MetroApp::planJourney() {
    ui::heading("PLAN A JOURNEY", "intelligent multi-criteria recommendation");
    std::cout << "\n";
    const auto src = askStation("From:");
    if (!src) return;
    const auto dst = askStation("To:  ", *src);
    if (!dst) return;
    const auto depart = askDeparture();
    if (!depart) return;
    const auto pref = askPreference();
    if (!pref) return;

    const JourneyContext ctx{*depart, settings_.day};
    ui::spinner("Running Dijkstra ×5, BFS and alternative-route search…", 900);
    const Recommendation rec = recommender_.recommend(*src, *dst, ctx, *pref);
    if (rec.options.empty()) {
        ui::error("No route exists between these stations.");
        return;
    }
    showRecommendation(rec, ctx, *pref);
    record(rec.options.front().route, preferenceName(*pref));
    afterJourney(rec, ctx);
}

void MetroApp::showRecommendation(const Recommendation& rec, const JourneyContext& ctx, Preference pref) {
    const ScoredRoute& best = rec.options.front();
    std::vector<std::string> badges = {"RECOMMENDED"};
    badges.insert(badges.end(), best.badges.begin(), best.badges.end());
    renderer_.journeyCard(best.route, "YOUR BEST ROUTE", ctx, badges);
    renderer_.reasons(rec.reasons);
    renderer_.timeline(best.route, ctx);
    renderer_.comparison(rec.options);
    std::cout << "\n  " << muted("Preference: ") << strong(preferenceName(pref)) << muted("  ·  ")
              << paint(str::fixed(rec.computeMs, 2) + " ms", pal::success, true)
              << muted(" to evaluate " + std::to_string(rec.statesExplored) + " graph states and " +
                       std::to_string(rec.options.size()) + " candidate routes")
              << "\n";
}

void MetroApp::afterJourney(const Recommendation& rec, const JourneyContext& ctx) {
    int shown = 0;
    while (true) {
        std::vector<ui::MenuItem> items;
        for (std::size_t i = 0; i < rec.options.size(); ++i) {
            const Route& r = rec.options[i].route;
            items.push_back({"View option " + std::to_string(i + 1),
                             str::formatDuration(r.totalMin) + " · " + std::to_string(r.interchanges) + " changes · ₹" +
                                 std::to_string(r.fare),
                             i == 0 ? pal::gold : pal::accent});
        }
        items.push_back({"Export HTML ticket", "save the displayed route as a shareable web page", pal::success});
        items.push_back({"Back to main menu", "", pal::muted});
        const int choice = ui::menu("WHAT NEXT?", items);
        const int exportIdx = static_cast<int>(rec.options.size());
        if (choice < 0 || choice == exportIdx + 1) return;
        if (choice == exportIdx) {
            const Route& r = rec.options[static_cast<std::size_t>(shown)].route;
            const std::string path = suggestedTicketName(r, graph_);
            if (exportJourneyHtml(r, graph_, ctx, path)) ui::success("Ticket saved to " + paint(path, pal::gold, true) + " — open it in any browser.");
            else ui::error("Could not write " + path);
            continue;
        }
        shown = choice;
        const ScoredRoute& opt = rec.options[static_cast<std::size_t>(choice)];
        renderer_.journeyCard(opt.route, "OPTION " + std::to_string(choice + 1), ctx, opt.badges);
        renderer_.timeline(opt.route, ctx);
    }
}

void MetroApp::routeByCriterion() {
    ui::heading("ROUTE BY CRITERION", "run one specific algorithm");
    std::cout << "\n";
    const Criterion all[] = {Criterion::Fastest, Criterion::Shortest, Criterion::LeastInterchange,
                             Criterion::Cheapest, Criterion::LeastCrowded, Criterion::FewestStops};
    const RGB colors[] = {pal::accent, RGB{255, 140, 90}, pal::accent2, pal::success, pal::warning, pal::accent3};
    std::vector<ui::MenuItem> items;
    for (int i = 0; i < 6; ++i) items.push_back({criterionName(all[i]), criterionAlgorithm(all[i]), colors[i]});
    const int choice = ui::menu("CHOOSE A CRITERION", items);
    if (choice < 0) return;
    const Criterion criterion = all[choice];

    const auto src = askStation("From:");
    if (!src) return;
    const auto dst = askStation("To:  ", *src);
    if (!dst) return;
    const auto depart = askDeparture();
    if (!depart) return;

    const JourneyContext ctx{*depart, settings_.day};
    ui::spinner(std::string("Searching with ") + criterionAlgorithm(criterion) + "…", 600);
    const auto route = finder_.find(*src, *dst, criterion, ctx);
    if (!route) {
        ui::error("No route exists between these stations.");
        return;
    }
    renderer_.journeyCard(*route, std::string(criterionName(criterion)) + " ROUTE", ctx, {});
    std::cout << "  " << muted("Algorithm: ") << strong(criterionAlgorithm(criterion)) << muted("  ·  states settled: ")
              << paint(std::to_string(finder_.lastExplored()), pal::success, true) << "\n";
    renderer_.timeline(*route, ctx);
    record(*route, criterionName(criterion));

    const std::vector<ui::MenuItem> next = {{"Export HTML ticket", "save this route as a web page", pal::success},
                                            {"Back to main menu", "", pal::muted}};
    if (ui::menu("WHAT NEXT?", next) == 0) {
        const std::string path = suggestedTicketName(*route, graph_);
        if (exportJourneyHtml(*route, graph_, ctx, path)) ui::success("Ticket saved to " + paint(path, pal::gold, true));
        else ui::error("Could not write " + path);
    }
}

void MetroApp::stationExplorer() {
    ui::heading("STATION EXPLORER", "type any part of a name — powered by a Trie");
    std::cout << "\n";
    const auto id = askStation("Station:");
    if (!id) return;
    const JourneyContext ctx{currentMinute(), settings_.day};
    renderer_.stationCard(*id, ctx);
    renderer_.crowdForecast(*id, ctx);
}

void MetroApp::linesExplorer() {
    renderer_.linesOverview();
    std::vector<ui::MenuItem> items;
    for (std::size_t i = 0; i < graph_.lineCount(); ++i) {
        const MetroLine& l = graph_.line(static_cast<int>(i));
        items.push_back({l.name, graph_.station(l.stations.front()).name + " ⇄ " + graph_.station(l.stations.back()).name, l.color});
    }
    const int choice = ui::menu("OPEN A LINE DIAGRAM", items);
    if (choice >= 0) renderer_.lineDiagram(choice);
}

void MetroApp::crowdHeatmap() {
    const auto depart = askDeparture();
    if (!depart) return;
    renderer_.heatmap(JourneyContext{*depart, settings_.day}, 15);
}

void MetroApp::showHistory() {
    renderer_.history(history_);
    if (history_.records().empty()) return;
    const std::vector<ui::MenuItem> items = {{"Back", "", pal::muted}, {"Clear history", "remove all saved journeys", pal::danger}};
    if (ui::menu("", items) == 1) {
        history_.clear();
        ui::success("History cleared.");
    }
}

void MetroApp::networkInfo() {
    renderer_.networkStats(trie_);
    renderer_.fareChart();
}

void MetroApp::settingsMenu() {
    const std::vector<ui::MenuItem> items = {
        {"Day type", std::string("currently ") + dayTypeName(settings_.day) + " (affects fares & crowds)", pal::accent},
        {"Animations", settings_.animations ? "currently on" : "currently off", pal::accent2},
        {"Back", "", pal::muted},
    };
    const int choice = ui::menu("SETTINGS", items);
    if (choice == 0) {
        settings_.day = settings_.day == DayType::Weekday ? DayType::Sunday : DayType::Weekday;
        ui::success(std::string("Day type set to ") + dayTypeName(settings_.day));
    } else if (choice == 1) {
        settings_.animations = !settings_.animations;
        term::setAnimations(settings_.animations);
        ui::success(std::string("Animations ") + (settings_.animations ? "enabled" : "disabled"));
    }
}

void MetroApp::record(const Route& route, const std::string& mode) {
    JourneyRecord r;
    r.timestamp = JourneyHistory::nowStamp();
    r.from = graph_.station(route.stations.front()).name;
    r.to = graph_.station(route.stations.back()).name;
    r.mode = mode;
    r.minutes = route.totalMin;
    r.fare = route.fare;
    r.interchanges = route.interchanges;
    history_.add(r);
}

} // namespace metro
