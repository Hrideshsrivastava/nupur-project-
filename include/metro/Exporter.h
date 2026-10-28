#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Exporter.h — saves a journey as a self-contained, shareable HTML ticket.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/MetroGraph.h"
#include "metro/RouteFinder.h"

#include <string>

namespace metro {

/// Write `route` as a styled HTML page to `path`. Returns false on I/O error.
bool exportJourneyHtml(const Route& route, const MetroGraph& graph, const JourneyContext& ctx,
                       const std::string& path);

/// A file name such as "journey_rajiv-chowk_to_noida-sector-62.html".
std::string suggestedTicketName(const Route& route, const MetroGraph& graph);

} // namespace metro
