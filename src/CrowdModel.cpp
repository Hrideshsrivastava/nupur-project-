#include "metro/CrowdModel.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace metro {

namespace {

// Share of peak ridership at the start of every hour (00:00 … 23:00).
constexpr std::array<double, 24> kWeekdayProfile = {
    0.02, 0.01, 0.01, 0.01, 0.02, 0.08, 0.25, 0.55, 0.90, 1.00, 0.80, 0.55,
    0.45, 0.45, 0.45, 0.50, 0.65, 0.90, 1.00, 0.85, 0.60, 0.40, 0.20, 0.08,
};
constexpr std::array<double, 24> kSundayProfile = {
    0.02, 0.01, 0.01, 0.01, 0.02, 0.05, 0.12, 0.22, 0.32, 0.42, 0.52, 0.60,
    0.65, 0.66, 0.65, 0.66, 0.70, 0.72, 0.70, 0.62, 0.50, 0.35, 0.18, 0.06,
};

double wrapMinute(double minuteOfDay) {
    double m = std::fmod(minuteOfDay, static_cast<double>(kMinutesPerDay));
    return m < 0 ? m + kMinutesPerDay : m;
}

} // namespace

CrowdModel::CrowdModel(const MetroGraph& graph) : graph_(graph) {}

double CrowdModel::demand(double minuteOfDay, DayType day) {
    const auto& profile = day == DayType::Weekday ? kWeekdayProfile : kSundayProfile;
    const double m = wrapMinute(minuteOfDay);
    const int hour = static_cast<int>(m / 60.0) % 24;
    const double t = (m - hour * 60.0) / 60.0;
    const double a = profile[static_cast<std::size_t>(hour)];
    const double b = profile[static_cast<std::size_t>((hour + 1) % 24)];
    return a + (b - a) * t;
}

double CrowdModel::level(int station, double minuteOfDay, DayType day) const {
    const Station& s = graph_.station(station);
    const double d = demand(minuteOfDay, day);
    const double popularity = 0.22 + 0.78 * (s.footfall / 10.0);

    // Deterministic per-station/per-hour jitter (±4%) so stations of equal
    // popularity do not look identical.
    const unsigned hour = static_cast<unsigned>(wrapMinute(minuteOfDay) / 60.0);
    const unsigned hash = static_cast<unsigned>(station) * 2654435761u + hour * 40503u;
    const double jitter = ((hash % 1000u) / 1000.0 - 0.5) * 8.0;

    return std::clamp(popularity * d * 100.0 + jitter * d, 0.0, 100.0);
}

std::vector<double> CrowdModel::forecast(int station, DayType day) const {
    std::vector<double> values(24);
    for (int h = 0; h < 24; ++h) values[static_cast<std::size_t>(h)] = level(station, h * 60.0 + 30.0, day);
    return values;
}

bool CrowdModel::isPeak(double minuteOfDay, DayType day) {
    if (day != DayType::Weekday) return false;
    const double m = wrapMinute(minuteOfDay);
    return (m >= 8 * 60 && m < 10 * 60 + 30) || (m >= 17 * 60 && m < 19 * 60 + 30);
}

const char* CrowdModel::label(double level) {
    if (level < 25) return "Low";
    if (level < 50) return "Moderate";
    if (level < 75) return "High";
    return "Packed";
}

} // namespace metro
