#include "metro/Route.h"

namespace metro {

std::string Route::signature() const {
    std::string key;
    for (std::size_t i = 0; i < stations.size(); ++i) {
        key += std::to_string(stations[i]);
        if (i < hopLines.size()) key += ':' + std::to_string(hopLines[i]) + '>';
    }
    return key;
}

double Route::co2SavedKg() const {
    // ≈192 g CO₂/km for an average petrol car vs ≈35 g per passenger-km by metro.
    return distanceKm * (0.192 - 0.035);
}

} // namespace metro
