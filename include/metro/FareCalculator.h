#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  FareCalculator.h — distance-slab fares (DMRC style) + Airport Express.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/MetroGraph.h"
#include "metro/Route.h"

#include <vector>

namespace metro {

struct FareBreakdown {
    double regularKm = 0.0; // km on regular lines
    double premiumKm = 0.0; // km on the Airport Express
    int regular = 0;
    int premium = 0;
    int total = 0;
    int smartCard = 0;      // total with 10% smart-card discount
};

class FareCalculator {
public:
    struct Slab {
        double upToKm; // inclusive upper bound of the slab
        int weekday;   // token fare Monday–Saturday
        int sunday;    // token fare on Sundays / national holidays
    };

    /// The distance slabs in ascending order.
    static const std::vector<Slab>& slabs();

    /// Fare on the regular network for `km` kilometres.
    static int slabFare(double km, DayType day);

    /// Fare on the Airport Express for `km` kilometres.
    static int premiumFare(double km);

    /// Fare with the smart-card discount applied.
    static int smartCardFare(int fare);

    /// Full fare breakdown for a route.
    static FareBreakdown compute(const Route& route, const MetroGraph& graph, DayType day);
};

} // namespace metro
