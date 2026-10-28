#include "metro/FareCalculator.h"

#include <cmath>

namespace metro {

const std::vector<FareCalculator::Slab>& FareCalculator::slabs() {
    static const std::vector<Slab> table = {
        {2.0, 11, 11}, {5.0, 21, 11}, {12.0, 32, 21},
        {21.0, 43, 32}, {32.0, 54, 43}, {1e9, 64, 54},
    };
    return table;
}

int FareCalculator::slabFare(double km, DayType day) {
    if (km <= 0.0) return 0;
    for (const Slab& s : slabs())
        if (km <= s.upToKm) return day == DayType::Weekday ? s.weekday : s.sunday;
    return slabs().back().weekday;
}

int FareCalculator::premiumFare(double km) {
    if (km <= 0.0) return 0;
    // Base fare + per-km rate, capped like the real Airport Line.
    const int fare = 20 + static_cast<int>(std::ceil(km) * 1.8);
    return fare > 75 ? 75 : fare;
}

int FareCalculator::smartCardFare(int fare) {
    return static_cast<int>(std::lround(fare * 0.9));
}

FareBreakdown FareCalculator::compute(const Route& route, const MetroGraph& graph, DayType day) {
    FareBreakdown f;
    for (const Segment& seg : route.segments) {
        if (seg.isWalk()) continue;
        if (graph.line(seg.line).premium) f.premiumKm += seg.distanceKm;
        else                              f.regularKm += seg.distanceKm;
    }
    f.regular = slabFare(f.regularKm, day);
    f.premium = premiumFare(f.premiumKm);
    f.total = f.regular + f.premium;
    f.smartCard = smartCardFare(f.total);
    return f;
}

} // namespace metro
