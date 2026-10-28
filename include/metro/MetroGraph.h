#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  MetroGraph.h — the metro network as a weighted, undirected graph stored in
//  an adjacency list, with an O(1) hash-map index from station name to id.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/Station.h"

#include <cstddef>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace metro {

class MetroGraph {
public:
    /// Register a new line and return its index.
    int addLine(const std::string& code, const std::string& name, RGB color, double speedKmph,
                bool premium = false);

    /// Add a station (or return the id of an existing one with the same name).
    int addStation(const std::string& name);

    /// Connect two stations on a line in both directions.
    void addConnection(int a, int b, int line, double distanceKm);

    /// Join two stations by a pedestrian skywalk (an edge with line == -1).
    void addWalkTransfer(const std::string& a, const std::string& b, double meters,
                         double minutes);

    /// Create every station of a line in order. Each pair is
    /// {station name, distance in km from the previous station}.
    void buildLine(int line, const std::vector<std::pair<std::string, double>>& stops);

    /// Override the popularity (1..10) of a station used by the crowd model.
    void setFootfall(const std::string& name, int footfall);

    /// Give every station without an explicit footfall a default based on
    /// how many lines serve it. Call once after the network is built.
    void finalizeFootfall();

    /// Hash-map lookup, tolerant to case/spacing/punctuation. -1 if unknown.
    int findStation(const std::string& name) const;

    /// Find a line by its code ("BL") or name ("Blue Line"). -1 if unknown.
    int findLine(const std::string& codeOrName) const;

    /// The edge a->b travelling on `line` (-1 = walk), or nullptr.
    const Edge* edgeBetween(int a, int b, int line) const;

    /// Hop count (number of edges) from `source` to every station via BFS.
    std::vector<int> bfsHops(int source) const;

    /// True if every station can be reached from station 0 (BFS).
    bool isConnected() const;

    std::vector<int> interchangeStations() const;

    const Station& station(int id) const { return stations_.at(static_cast<std::size_t>(id)); }
    const std::vector<Station>& stations() const { return stations_; }
    const std::vector<Edge>& neighbors(int id) const { return adj_.at(static_cast<std::size_t>(id)); }
    const MetroLine& line(int idx) const { return lines_.at(static_cast<std::size_t>(idx)); }
    const std::vector<MetroLine>& lines() const { return lines_; }

    std::size_t stationCount() const { return stations_.size(); }
    std::size_t lineCount() const { return lines_.size(); }
    std::size_t connectionCount() const { return connections_; }
    double totalTrackKm() const;

private:
    std::vector<Station> stations_;
    std::vector<std::vector<Edge>> adj_; // adjacency list, indexed by station id
    std::vector<MetroLine> lines_;
    std::unordered_map<std::string, int> index_; // normalised name -> station id
    std::size_t connections_ = 0;
};

} // namespace metro
