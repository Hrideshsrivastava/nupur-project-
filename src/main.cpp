// ─────────────────────────────────────────────────────────────────────────────
//  Metro Route Navigation System — entry point.
//
//  Usage:
//    metro_navigator                       interactive mode
//    metro_navigator --from A --to B       one-shot recommendation
//                    [--time HH:MM] [--pref balanced|fast|budget|comfort|simple]
//    Options: --sunday --no-color --no-anim --truecolor --help
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/App.h"
#include "metro/Terminal.h"

#include <iostream>
#include <string>

namespace {

void printHelp() {
    std::cout <<
        "Metro Route Navigation System — Delhi NCR\n\n"
        "Usage:\n"
        "  metro_navigator                         start the interactive app\n"
        "  metro_navigator --from <station> --to <station> [--time HH:MM] [--pref <p>]\n\n"
        "Options:\n"
        "  --from <name>     origin station (partial names work: \"rajiv\")\n"
        "  --to <name>       destination station\n"
        "  --time <HH:MM>    departure time (default: now)\n"
        "  --pref <p>        balanced | fast | budget | comfort | simple\n"
        "  --sunday          use Sunday / holiday fares and crowd levels\n"
        "  --no-color        plain text output\n"
        "  --truecolor       force 24-bit colour\n"
        "  --no-anim         disable animations\n"
        "  --history <file>  where to store journey history (default metro_history.txt)\n"
        "  -h, --help        show this help\n";
}

} // namespace

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    metro::term::init();

    metro::Settings settings;
    std::string from, to, time, pref = "balanced";
    for (int i = 1; i < argc; ++i) {
        const std::string arg = argv[i];
        auto next = [&]() -> std::string { return i + 1 < argc ? argv[++i] : ""; };
        if (arg == "-h" || arg == "--help") {
            printHelp();
            return 0;
        } else if (arg == "--from") from = next();
        else if (arg == "--to") to = next();
        else if (arg == "--time") time = next();
        else if (arg == "--pref") pref = next();
        else if (arg == "--history") settings.historyFile = next();
        else if (arg == "--sunday") settings.day = metro::DayType::Sunday;
        else if (arg == "--no-color") metro::term::setColorMode(metro::term::ColorMode::None);
        else if (arg == "--truecolor") metro::term::setColorMode(metro::term::ColorMode::TrueColor);
        else if (arg == "--no-anim") settings.animations = false;
        else {
            std::cerr << "Unknown option: " << arg << "\n\n";
            printHelp();
            return 2;
        }
    }
    if (!metro::term::animations()) settings.animations = false;
    metro::term::setAnimations(settings.animations);

    metro::MetroApp app(settings);
    int code = 0;
    if (!from.empty() || !to.empty()) {
        if (from.empty() || to.empty()) {
            std::cerr << "Both --from and --to are required for a one-shot query.\n";
            return 2;
        }
        code = app.runQuick(from, to, time, pref);
    } else {
        code = app.run();
    }
    metro::term::showCursor();
    std::cout << std::flush;
    return code;
}
