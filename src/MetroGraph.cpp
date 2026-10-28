#include "metro/MetroGraph.h"

#include "metro/StringUtil.h"

#include <algorithm>
#include <queue>
#include <stdexcept>

namespace metro {

int MetroGraph::addLine(const std::string& code, const std::string& name, RGB color,
                        double speedKmph, bool premium) {
    MetroLine line;
    line.code = code;
    line.name = name;
    line.color = color;
    line.speedKmph = speedKmph;
    line.premium = premium;
    lines_.push_back(line);
    return static_cast<int>(lines_.size()) - 1;
}

int MetroGraph::addStation(const std::string& name) {
    const std::string key = str::normalizeKey(name);
    auto it = index_.find(key);
    if (it != index_.end()) return it->second;

    Station s;
    s.id = static_cast<int>(stations_.size());
    s.name = name;
    stations_.push_back(s);
    adj_.emplace_back();
    index_.emplace(key, s.id);
    return s.id;
}

void MetroGraph::addConnection(int a, int b, int line, double distanceKm) {
    const MetroLine& l = lines_.at(static_cast<std::size_t>(line));
    // Running time at the line's average speed plus the halt at the next station.
    const double minutes = distanceKm / l.speedKmph * 60.0 + kDwellMin;
    adj_[static_cast<std::size_t>(a)].push_back(Edge{b, line, distanceKm, minutes});
    adj_[static_cast<std::size_t>(b)].push_back(Edge{a, line, distanceKm, minutes});
    lines_[static_cast<std::size_t>(line)].lengthKm += distanceKm;
    ++connections_;
}

void MetroGraph::addWalkTransfer(const std::string& a, const std::string& b, double meters,
                                 double minutes) {
    const int ia = findStation(a), ib = findStation(b);
    if (ia < 0 || ib < 0) throw std::invalid_argument("addWalkTransfer: unknown station");
    const double km = meters / 1000.0;
    adj_[static_cast<std::size_t>(ia)].push_back(Edge{ib, -1, km, minutes});
    adj_[static_cast<std::size_t>(ib)].push_back(Edge{ia, -1, km, minutes});
    stations_[static_cast<std::size_t>(ia)].hasWalkLink = true;
    stations_[static_cast<std::size_t>(ib)].hasWalkLink = true;
    ++connections_;
}

void MetroGraph::buildLine(int line, const std::vector<std::pair<std::string, double>>& stops) {
    int previous = -1;
    for (const auto& [name, kmFromPrevious] : stops) {
        const int id = addStation(name);
        auto& served = stations_[static_cast<std::size_t>(id)].lines;
        if (std::find(served.begin(), served.end(), line) == served.end()) served.push_back(line);
        lines_[static_cast<std::size_t>(line)].stations.push_back(id);
        if (previous >= 0) addConnection(previous, id, line, kmFromPrevious);
        previous = id;
    }
}

void MetroGraph::setFootfall(const std::string& name, int footfall) {
    const int id = findStation(name);
    if (id < 0) throw std::invalid_argument("setFootfall: unknown station " + name);
    stations_[static_cast<std::size_t>(id)].footfall = std::clamp(footfall, 1, 10);
}

void MetroGraph::finalizeFootfall() {
    for (Station& s : stations_) {
        if (s.footfall > 0) continue;
        int value = 3 + 2 * (static_cast<int>(s.lines.size()) - 1) + (s.hasWalkLink ? 1 : 0);
        s.footfall = std::clamp(value, 1, 10);
    }
}

int MetroGraph::findStation(const std::string& name) const {
    auto it = index_.find(str::normalizeKey(name));
    return it == index_.end() ? -1 : it->second;
}

int MetroGraph::findLine(const std::string& codeOrName) const {
    const std::string key = str::normalizeKey(codeOrName);
    for (std::size_t i = 0; i < lines_.size(); ++i) {
        if (str::normalizeKey(lines_[i].code) == key || str::normalizeKey(lines_[i].name) == key)
            return static_cast<int>(i);
    }
    return -1;
}

const Edge* MetroGraph::edgeBetween(int a, int b, int line) const {
    for (const Edge& e : adj_.at(static_cast<std::size_t>(a)))
        if (e.to == b && e.line == line) return &e;
    return nullptr;
}

std::vector<int> MetroGraph::bfsHops(int source) const {
    std::vector<int> hops(stations_.size(), -1);
    if (source < 0 || static_cast<std::size_t>(source) >= stations_.size()) return hops;
    std::queue<int> frontier;
    hops[static_cast<std::size_t>(source)] = 0;
    frontier.push(source);
    while (!frontier.empty()) {
        const int u = frontier.front();
        frontier.pop();
        for (const Edge& e : adj_[static_cast<std::size_t>(u)]) {
            if (hops[static_cast<std::size_t>(e.to)] == -1) {
                hops[static_cast<std::size_t>(e.to)] = hops[static_cast<std::size_t>(u)] + 1;
                frontier.push(e.to);
            }
        }
    }
    return hops;
}

bool MetroGraph::isConnected() const {
    if (stations_.empty()) return true;
    const std::vector<int> hops = bfsHops(0);
    return std::none_of(hops.begin(), hops.end(), [](int h) { return h < 0; });
}

std::vector<int> MetroGraph::interchangeStations() const {
    std::vector<int> ids;
    for (const Station& s : stations_)
        if (s.isInterchange()) ids.push_back(s.id);
    return ids;
}

double MetroGraph::totalTrackKm() const {
    double km = 0.0;
    for (const MetroLine& l : lines_) km += l.lengthKm;
    return km;
}

} // namespace metro
