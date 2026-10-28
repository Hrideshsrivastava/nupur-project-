#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  MetroData.h — the built-in Delhi NCR metro network.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/MetroGraph.h"

namespace metro::data {

/// Builds the network: 12 lines, ~245 stations, interchanges and skywalks.
/// Inter-station distances are realistic approximations.
MetroGraph buildDelhiMetro();

} // namespace metro::data
