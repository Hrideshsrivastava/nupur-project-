#include "metro/UI.h"

#include "metro/StringUtil.h"
#include "metro/Terminal.h"

#include <algorithm>
#include <cmath>
#include <iostream>

namespace metro::ui {

namespace {

// Split a UTF-8 string into code points (each kept as its byte sequence).
std::vector<std::string> codePoints(const std::string& s) {
    std::vector<std::string> out;
    for (std::size_t i = 0; i < s.size();) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        std::size_t len = 1;
        if ((c & 0xE0) == 0xC0) len = 2;
        else if ((c & 0xF0) == 0xE0) len = 3;
        else if ((c & 0xF8) == 0xF0) len = 4;
        out.push_back(s.substr(i, len));
        i += len;
    }
    return out;
}

constexpr RGB kTrack{52, 58, 82}; // colour of the empty part of a meter
const char* const kEighths[] = {"", "▏", "▎", "▍", "▌", "▋", "▊", "▉"};

// Shared bar renderer: `colorAt(i)` gives the colour of cell i.
template <typename ColorFn>
std::string bar(double fraction, int width, ColorFn colorAt) {
    fraction = std::clamp(fraction, 0.0, 1.0);
    const double cells = fraction * width;
    const int full = static_cast<int>(cells);
    const int eighths = static_cast<int>(std::lround((cells - full) * 8));
    std::string out;
    int used = 0;
    for (int i = 0; i < full; ++i, ++used) out += term::fg(colorAt(i)) + "█";
    if (used < width && eighths > 0) {
        out += term::fg(colorAt(full)) + (eighths >= 8 ? "█" : kEighths[eighths]);
        ++used;
    }
    if (used < width) out += term::fg(kTrack) + repeat("░", static_cast<std::size_t>(width - used));
    return out + term::reset();
}

} // namespace

int layoutWidth() { return std::clamp(term::columns() - 4, 64, 104); }

std::size_t visibleWidth(const std::string& s) {
    std::size_t width = 0;
    for (std::size_t i = 0; i < s.size(); ++i) {
        const unsigned char c = static_cast<unsigned char>(s[i]);
        if (c == 0x1b) { // skip a CSI escape sequence: ESC [ ... final-byte
            ++i;
            if (i < s.size() && s[i] == '[') {
                ++i;
                while (i < s.size() && !(s[i] >= 0x40 && s[i] <= 0x7e)) ++i;
            }
            continue;
        }
        if ((c & 0xC0) != 0x80) ++width; // count only UTF-8 lead bytes
    }
    return width;
}

std::string pad(const std::string& s, std::size_t width, Align align) {
    const std::size_t w = visibleWidth(s);
    if (w >= width) return s;
    const std::size_t gap = width - w;
    switch (align) {
        case Align::Left: return s + std::string(gap, ' ');
        case Align::Right: return std::string(gap, ' ') + s;
        case Align::Center: return std::string(gap / 2, ' ') + s + std::string(gap - gap / 2, ' ');
    }
    return s;
}

std::string truncate(const std::string& s, std::size_t width) {
    const auto cps = codePoints(s);
    if (cps.size() <= width) return s;
    std::string out;
    for (std::size_t i = 0; i + 1 < width && i < cps.size(); ++i) out += cps[i];
    return out + "…";
}

std::string repeat(const std::string& s, std::size_t n) {
    std::string out;
    out.reserve(s.size() * n);
    for (std::size_t i = 0; i < n; ++i) out += s;
    return out;
}

RGB lerp(RGB a, RGB b, double t) {
    t = std::clamp(t, 0.0, 1.0);
    auto mix = [t](std::uint8_t x, std::uint8_t y) {
        return static_cast<std::uint8_t>(std::lround(x + (y - x) * t));
    };
    return RGB{mix(a.r, b.r), mix(a.g, b.g), mix(a.b, b.b)};
}

std::string paint(const std::string& text, RGB color, bool isBold) {
    if (term::colorMode() == term::ColorMode::None) return text;
    return (isBold ? term::bold() : std::string()) + term::fg(color) + text + term::reset();
}

std::string muted(const std::string& text) { return paint(text, palette::muted); }
std::string strong(const std::string& text) { return paint(text, palette::text, true); }

std::string gradient(const std::string& text, RGB from, RGB to, bool isBold) {
    if (term::colorMode() == term::ColorMode::None) return text;
    const auto cps = codePoints(text);
    std::string out = isBold ? term::bold() : "";
    const double span = cps.size() > 1 ? static_cast<double>(cps.size() - 1) : 1.0;
    for (std::size_t i = 0; i < cps.size(); ++i) {
        if (cps[i] == " ") {
            out += " ";
            continue;
        }
        out += term::fg(lerp(from, to, static_cast<double>(i) / span)) + cps[i];
    }
    return out + term::reset();
}

RGB crowdColor(double level) {
    constexpr RGB green{80, 220, 140}, yellow{255, 205, 60}, orange{255, 140, 50},
        red{255, 70, 90};
    level = std::clamp(level, 0.0, 100.0);
    if (level < 45) return lerp(green, yellow, level / 45.0);
    if (level < 72) return lerp(yellow, orange, (level - 45) / 27.0);
    return lerp(orange, red, (level - 72) / 28.0);
}

std::string badge(const std::string& text, RGB background, RGB foreground) {
    if (term::colorMode() == term::ColorMode::None) return "[" + text + "]";
    return term::bg(background) + term::fg(foreground) + term::bold() + " " + text + " " +
           term::reset();
}

std::string meter(double fraction, int width, RGB from, RGB to) {
    return bar(fraction, width, [&](int i) {
        return lerp(from, to, width > 1 ? static_cast<double>(i) / (width - 1) : 0.0);
    });
}

std::string crowdMeter(double level, int width) {
    return bar(level / 100.0, width, [&](int i) {
        return crowdColor(100.0 * (i + 0.5) / std::max(1, width));
    });
}

void heading(const std::string& title, const std::string& subtitle) {
    const int w = layoutWidth();
    std::cout << "\n  " << paint("▍", palette::accent) << " "
              << gradient(title, palette::accent, palette::accent2, true);
    if (!subtitle.empty()) std::cout << "  " << muted(subtitle);
    std::cout << "\n  " << gradient(repeat("─", static_cast<std::size_t>(w)), palette::accent2, palette::border)
              << "\n";
}

void panel(const std::string& title, const std::vector<std::string>& lines, RGB border, int width) {
    const int w = width > 0 ? width : layoutWidth();
    const std::string b = term::fg(border);
    const std::string r = term::reset();
    const std::size_t inner = static_cast<std::size_t>(w - 4);

    std::cout << "  " << b << "╭";
    if (title.empty()) {
        std::cout << repeat("─", static_cast<std::size_t>(w - 2));
    } else {
        const std::size_t tw = visibleWidth(title);
        const std::size_t dashes = static_cast<std::size_t>(std::max<int>(1, w - 5 - static_cast<int>(tw)));
        std::cout << "─ " << r << title << b << " " << repeat("─", dashes);
    }
    std::cout << "╮" << r << "\n";
    for (const std::string& line : lines)
        std::cout << "  " << b << "│" << r << " " << pad(line, inner) << " " << b << "│" << r << "\n";
    std::cout << "  " << b << "╰" << repeat("─", static_cast<std::size_t>(w - 2)) << "╯" << r << "\n";
}

void rule(RGB color) {
    std::cout << "  " << paint(repeat("─", static_cast<std::size_t>(layoutWidth())), color) << "\n";
}

void success(const std::string& m) { std::cout << "  " << paint("✔", palette::success, true) << " " << m << "\n"; }
void warn(const std::string& m) { std::cout << "  " << paint("▲", palette::warning, true) << " " << m << "\n"; }
void error(const std::string& m) { std::cout << "  " << paint("✖", palette::danger, true) << " " << paint(m, palette::danger) << "\n"; }
void info(const std::string& m) { std::cout << "  " << paint("●", palette::accent, true) << " " << m << "\n"; }

void spinner(const std::string& message, int durationMs) {
    if (!term::animations()) return;
    static const char* const frames[] = {"⠋", "⠙", "⠹", "⠸", "⠼", "⠴", "⠦", "⠧", "⠇", "⠏"};
    term::hideCursor();
    const int steps = std::max(1, durationMs / 70);
    for (int i = 0; i < steps; ++i) {
        const RGB c = lerp(palette::accent, palette::accent2, (i % 10) / 9.0);
        std::cout << "\r  " << paint(frames[i % 10], c, true) << " " << muted(message) << std::flush;
        term::sleepMs(70);
    }
    std::cout << "\r\x1b[2K" << std::flush;
    term::showCursor();
}

std::optional<std::string> prompt(const std::string& label) {
    std::cout << "  " << paint("›", palette::accent, true) << " " << strong(label) << " "
              << term::fg(palette::gold) << std::flush;
    std::string line;
    const bool ok = static_cast<bool>(std::getline(std::cin, line));
    std::cout << term::reset();
    if (!ok) {
        std::cout << "\n";
        return std::nullopt;
    }
    return str::trim(line);
}

void pressEnter() {
    if (!term::interactive()) return;
    std::cout << "\n  " << muted("Press any key to continue…") << std::flush;
    {
        term::RawMode raw;
        term::readKey();
    }
    std::cout << "\r\x1b[2K";
}

void drawBlock(const std::vector<std::string>& lines) {
    std::string out;
    for (std::size_t i = 0; i < lines.size(); ++i) {
        out += "\r\x1b[2K" + lines[i];
        if (i + 1 < lines.size()) out += "\n";
    }
    if (lines.size() > 1) out += "\x1b[" + std::to_string(lines.size() - 1) + "A";
    out += "\r";
    std::cout << out << std::flush;
}

void endBlock(std::size_t lineCount) {
    if (lineCount > 1) std::cout << "\x1b[" << lineCount - 1 << "B";
    std::cout << "\n" << std::flush;
}

int menu(const std::string& title, const std::vector<MenuItem>& items, int initial) {
    if (items.empty()) return -1;
    if (!title.empty()) std::cout << "\n  " << muted(title) << "\n\n";

    std::size_t labelWidth = 0;
    for (const MenuItem& it : items) labelWidth = std::max(labelWidth, visibleWidth(it.label));

    if (!term::interactive()) {
        for (std::size_t i = 0; i < items.size(); ++i)
            std::cout << "    " << paint(std::to_string(i + 1), items[i].color, true) << "  "
                      << pad(items[i].label, labelWidth) << "  " << muted(items[i].hint) << "\n";
        while (true) {
            auto in = prompt("Select (1-" + std::to_string(items.size()) + ", 0 = back):");
            if (!in || *in == "0" || *in == "q") return -1;
            try {
                const int v = std::stoi(*in);
                if (v >= 1 && v <= static_cast<int>(items.size())) return v - 1;
            } catch (...) {
            }
            error("Please enter a number between 1 and " + std::to_string(items.size()));
        }
    }

    int selected = std::clamp(initial, 0, static_cast<int>(items.size()) - 1);
    auto build = [&]() {
        std::vector<std::string> lines;
        for (std::size_t i = 0; i < items.size(); ++i) {
            const MenuItem& it = items[i];
            const std::string number = std::to_string(i + 1);
            if (static_cast<int>(i) == selected) {
                lines.push_back("  " + paint("❯", it.color, true) + " " + term::bg(RGB{34, 40, 64}) +
                                term::fg(it.color) + term::bold() + " " + pad(number, 2) + " " +
                                pad(it.label, labelWidth) + " " + term::reset() + "  " +
                                paint(it.hint, palette::text));
            } else {
                lines.push_back("    " + muted(pad(number, 2)) + "  " +
                                paint(pad(it.label, labelWidth), palette::text) + "  " + muted(it.hint));
            }
        }
        lines.push_back("");
        lines.push_back("    " + muted("↑/↓ move   Enter select   1-9 jump   Esc back"));
        return lines;
    };

    term::hideCursor();
    term::RawMode raw;
    auto lines = build();
    drawBlock(lines);
    int result = -2;
    while (result == -2) {
        const int key = term::readKey();
        if (key == term::KeyUp || key == 'k') {
            selected = (selected + static_cast<int>(items.size()) - 1) % static_cast<int>(items.size());
        } else if (key == term::KeyDown || key == 'j' || key == term::KeyTab) {
            selected = (selected + 1) % static_cast<int>(items.size());
        } else if (key == term::KeyEnter) {
            result = selected;
        } else if (key >= '1' && key <= '9' && key - '1' < static_cast<int>(items.size())) {
            selected = key - '1';
            result = selected;
        } else if (key == term::KeyEscape || key == term::KeyCtrlC || key == term::KeyEOF ||
                   key == 'q' || key == '0') {
            result = -1;
        }
        lines = build();
        drawBlock(lines);
    }
    endBlock(lines.size());
    term::showCursor();
    return result;
}

void Table::addColumn(const std::string& header, Align align) {
    headers_.push_back(header);
    aligns_.push_back(align);
}

void Table::addRow(const std::vector<std::string>& cells) { rows_.push_back(cells); }

void Table::print(RGB accent) const {
    const std::size_t cols = headers_.size();
    std::vector<std::size_t> widths(cols, 0);
    for (std::size_t c = 0; c < cols; ++c) {
        widths[c] = visibleWidth(headers_[c]);
        for (const auto& row : rows_)
            if (c < row.size()) widths[c] = std::max(widths[c], visibleWidth(row[c]));
    }
    const std::string b = term::fg(palette::border);
    const std::string r = term::reset();
    auto border = [&](const char* left, const char* mid, const char* right) {
        std::string line = "  " + b + left;
        for (std::size_t c = 0; c < cols; ++c) {
            line += repeat("─", widths[c] + 2);
            line += c + 1 < cols ? mid : right;
        }
        return line + r + "\n";
    };

    std::cout << border("╭", "┬", "╮");
    std::cout << "  " << b << "│" << r;
    for (std::size_t c = 0; c < cols; ++c)
        std::cout << " " << pad(paint(headers_[c], accent, true), widths[c], aligns_[c]) << " " << b << "│" << r;
    std::cout << "\n" << border("├", "┼", "┤");
    for (const auto& row : rows_) {
        std::cout << "  " << b << "│" << r;
        for (std::size_t c = 0; c < cols; ++c) {
            const std::string cell = c < row.size() ? row[c] : "";
            std::cout << " " << pad(cell, widths[c], aligns_[c]) << " " << b << "│" << r;
        }
        std::cout << "\n";
    }
    std::cout << border("╰", "┴", "╯");
}

} // namespace metro::ui
