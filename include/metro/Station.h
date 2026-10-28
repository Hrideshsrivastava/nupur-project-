#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Station.h — the vertices (Station), edges (Edge) and lines (MetroLine) of
//  the metro graph.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/Types.h"

#include <string>
#include <vector>

namespace metro {

/// A metro line such as "Blue Line". Stations are stored in running order.
struct MetroLine {
    std::string code;          // short unique code, e.g. "BL"
    std::string name;          // display name, e.g. "Blue Line"
    RGB color;                 // official-ish line colour
    double speedKmph = 32.0;   // average running speed between stations
    bool premium = false;      // premium fare line (Airport Express)
    std::vector<int> stations; // station ids in running order
    double lengthKm = 0.0;     // total track length (sum of its edges)
};

/// A directed edge of the adjacency list. Every physical connection is stored
/// twice (a->b and b->a). `line == -1` marks a walking transfer between two
/// stations that are joined by a footbridge/skywalk instead of track.
struct Edge {
    int to = -1;
    int line = -1;
    double distanceKm = 0.0;
    double timeMin = 0.0;

    bool isWalk() const { return line < 0; }
};

/// A vertex of the graph.
struct Station {
    int id = -1;
    std::string name;
    std::vector<int> lines;   // indices of every line serving the station
    int footfall = 0;         // relative popularity 1..10 (drives crowd model)
    bool hasWalkLink = false; // connected to another station by a skywalk

    bool isInterchange() const { return lines.size() > 1 || hasWalkLink; }
};

} // namespace metro
