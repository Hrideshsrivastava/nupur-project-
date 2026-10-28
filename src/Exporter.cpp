#include "metro/Exporter.h"

#include "metro/CrowdModel.h"
#include "metro/StringUtil.h"

#include <cmath>
#include <cstdio>
#include <fstream>
#include <sstream>

namespace metro {

namespace {

std::string escapeHtml(const std::string& s) {
    std::string out;
    for (char c : s) {
        switch (c) {
            case '&': out += "&amp;"; break;
            case '<': out += "&lt;"; break;
            case '>': out += "&gt;"; break;
            case '"': out += "&quot;"; break;
            default: out += c;
        }
    }
    return out;
}

std::string hex(RGB c) {
    char buf[8];
    std::snprintf(buf, sizeof buf, "#%02x%02x%02x", c.r, c.g, c.b);
    return buf;
}

std::string slug(const std::string& s) {
    std::string out;
    for (char c : str::normalizeWords(s)) out += c == ' ' ? '-' : c;
    return out;
}

std::string crowdHex(double level) {
    if (level < 25) return "#50dc8c";
    if (level < 50) return "#ffcd3c";
    if (level < 75) return "#ff8c32";
    return "#ff465a";
}

} // namespace

std::string suggestedTicketName(const Route& route, const MetroGraph& graph) {
    return "journey_" + slug(graph.station(route.stations.front()).name) + "_to_" +
           slug(graph.station(route.stations.back()).name) + ".html";
}

bool exportJourneyHtml(const Route& route, const MetroGraph& graph, const JourneyContext& ctx,
                       const std::string& path) {
    if (route.empty()) return false;
    const std::string from = escapeHtml(graph.station(route.stations.front()).name);
    const std::string to = escapeHtml(graph.station(route.stations.back()).name);

    std::ostringstream rows;
    std::size_t idx = 0;
    for (std::size_t s = 0; s < route.segments.size(); ++s) {
        const Segment& seg = route.segments[s];
        const std::string color = seg.isWalk() ? "#7d849c" : hex(graph.line(seg.line).color);
        const std::string lineName = seg.isWalk() ? "Walk via skywalk" : escapeHtml(graph.line(seg.line).name);
        if (seg.transferBeforeMin > 0)
            rows << "<div class=\"change\">&#8644; Change to <b style=\"color:" << color << "\">" << lineName
                 << "</b> &middot; ~" << static_cast<int>(seg.transferBeforeMin) << " min</div>\n";
        rows << "<div class=\"seg\" style=\"--c:" << color << "\"><div class=\"seghead\"><span class=\"chip\">"
             << (seg.isWalk() ? "WALK" : escapeHtml(graph.line(seg.line).code)) << "</span>" << lineName
             << " <small>&middot; " << seg.stops() << " stop" << (seg.stops() == 1 ? "" : "s") << " &middot; "
             << str::formatDuration(seg.rideMin) << "</small></div>\n";
        for (std::size_t k = (s == 0 ? 0 : 1); k < seg.stations.size(); ++k) {
            const std::size_t i = idx + k;
            const bool major = k == 0 || k + 1 == seg.stations.size();
            const double c = route.crowd[i];
            rows << "<div class=\"stop" << (major ? " major" : "") << "\"><span class=\"t\">"
                 << str::formatClock(ctx.departMinute + route.etaMin[i]) << "</span><span class=\"dot\"></span><span class=\"n\">"
                 << escapeHtml(graph.station(route.stations[i]).name) << "</span><span class=\"crowd\"><i style=\"width:"
                 << static_cast<int>(std::lround(c)) << "%;background:" << crowdHex(c) << "\"></i></span><span class=\"p\">"
                 << static_cast<int>(std::lround(c)) << "%</span></div>\n";
        }
        rows << "</div>\n";
        idx += seg.stations.size() - 1;
    }

    std::ostringstream ribbon;
    for (const Segment& seg : route.segments) {
        const std::string color = seg.isWalk() ? "#7d849c" : hex(graph.line(seg.line).color);
        ribbon << "<i style=\"flex:" << std::max(0.5, seg.rideMin) << ";background:" << color << "\"></i>";
    }

    std::ofstream out(path);
    if (!out) return false;
    out << R"(<!doctype html>
<html lang="en"><head><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Metro Journey Ticket</title>
<style>
:root{--bg:#0b0d17;--card:#141828;--line:#262c45;--text:#e6e8f0;--muted:#7d849c;--accent:#00d2ff;--accent2:#aa6eff;--pink:#ff5faa}
*{box-sizing:border-box}body{margin:0;min-height:100vh;background:radial-gradient(circle at 20% 0%,#1b2350 0,var(--bg) 55%);color:var(--text);font:15px/1.5 "Segoe UI",system-ui,-apple-system,sans-serif;display:flex;justify-content:center;padding:32px 16px}
.card{width:100%;max-width:720px;background:var(--card);border:1px solid var(--line);border-radius:22px;overflow:hidden;box-shadow:0 30px 80px #0008}
header{padding:28px 32px;background:linear-gradient(120deg,#00d2ff22,#aa6eff22,#ff5faa22)}
.brand{letter-spacing:.35em;font-size:12px;color:var(--muted)}
h1{margin:6px 0 0;font-size:28px;background:linear-gradient(90deg,var(--accent),var(--accent2),var(--pink));-webkit-background-clip:text;background-clip:text;color:transparent}
.route{display:flex;justify-content:space-between;margin-top:20px;font-weight:700;font-size:18px;gap:12px}
.route span:last-child{text-align:right}.route small{display:block;color:var(--muted);font-weight:400;font-size:12px;letter-spacing:.1em}
.ribbon{display:flex;height:8px;border-radius:8px;overflow:hidden;margin-top:12px;gap:3px}
.tiles{display:grid;grid-template-columns:repeat(auto-fit,minmax(112px,1fr));border-top:1px solid var(--line);border-bottom:1px solid var(--line)}
.tile{padding:16px 20px;border-right:1px solid var(--line)}.tile b{display:block;font-size:19px;white-space:nowrap}.tile span{font-size:11px;letter-spacing:.15em;color:var(--muted)}
main{padding:24px 32px}
.seg{border-left:4px solid var(--c);margin-left:62px;padding:2px 0 8px 0;position:relative}
.seghead{margin:0 0 6px -66px;padding-left:84px;font-weight:600;color:var(--c)}.seghead small{color:var(--muted);font-weight:400}
.chip{background:var(--c);color:#0e101a;font-weight:800;font-size:11px;padding:2px 7px;border-radius:6px;margin-right:8px;position:absolute;left:-60px}
.stop{display:flex;align-items:center;gap:12px;margin-left:-66px;height:28px;color:var(--muted)}
.stop.major{color:var(--text);font-weight:600}
.t{width:44px;text-align:right;font-variant-numeric:tabular-nums;font-size:13px}
.dot{width:12px;height:12px;border-radius:50%;background:var(--card);border:3px solid var(--c);margin-left:2px}
.stop.major .dot{width:16px;height:16px;margin-left:0;background:var(--c)}
.n{flex:1}.crowd{width:90px;height:6px;border-radius:6px;background:#262c45;overflow:hidden}.crowd i{display:block;height:100%}
.p{width:38px;text-align:right;font-size:12px}
.change{margin:6px 0 10px;padding:8px 14px;border:1px dashed #ffbe3c88;border-radius:10px;color:#ffbe3c;font-size:13px}
footer{padding:16px 32px 28px;color:var(--muted);font-size:12px;display:flex;justify-content:space-between;flex-wrap:wrap;gap:8px}
</style></head><body><div class="card">
<header><div class="brand">METRO ROUTE NAVIGATION SYSTEM &middot; DELHI NCR</div><h1>Journey Ticket</h1>
<div class="route"><span><small>FROM &middot; )" << str::formatClock(ctx.departMinute) << "</small>" << from
        << "</span><span><small>TO &middot; " << str::formatClock(ctx.departMinute + route.totalMin) << "</small>" << to
        << "</span></div><div class=\"ribbon\">" << ribbon.str() << "</div></header>\n<div class=\"tiles\">"
        << "<div class=\"tile\"><b style=\"color:#00d2ff\">" << str::formatDuration(route.totalMin) << "</b><span>DURATION</span></div>"
        << "<div class=\"tile\"><b style=\"color:#ff8c5a\">" << str::fixed(route.distanceKm, 1) << " km</b><span>DISTANCE</span></div>"
        << "<div class=\"tile\"><b style=\"color:#ff5faa\">" << route.stops() << "</b><span>STOPS</span></div>"
        << "<div class=\"tile\"><b style=\"color:#aa6eff\">" << route.interchanges << "</b><span>CHANGES</span></div>"
        << "<div class=\"tile\"><b style=\"color:#50dc8c\">&#8377;" << route.fare << "</b><span>FARE</span></div>"
        << "<div class=\"tile\"><b style=\"color:" << crowdHex(route.avgCrowd) << "\">" << static_cast<int>(std::lround(route.avgCrowd))
        << "%</b><span>AVG CROWD</span></div></div>\n<main>\n" << rows.str() << "</main>\n<footer><span>"
        << dayTypeName(ctx.day) << " &middot; smart card &#8377;" << route.smartCardFare << " &middot; CO&#8322; saved &asymp; "
        << str::fixed(route.co2SavedKg(), 1) << " kg</span><span>Generated by Metro Navigator (C++)</span></footer>\n"
        << "</div></body></html>\n";
    return static_cast<bool>(out);
}

} // namespace metro
