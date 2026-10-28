#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  UI.h — reusable visual building blocks for the terminal: gradients,
//  rounded panels, tables, meters, badges, spinners and arrow-key menus.
//  Every function degrades gracefully when colours or animations are off.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/Types.h"

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace metro::ui {

enum class Align { Left, Right, Center };

/// The application's colour palette.
namespace palette {
constexpr RGB accent{0, 210, 255};    // cyan
constexpr RGB accent2{170, 110, 255}; // violet
constexpr RGB accent3{255, 95, 170};  // pink
constexpr RGB text{230, 232, 240};
constexpr RGB muted{125, 132, 156};
constexpr RGB border{72, 82, 112};
constexpr RGB success{80, 220, 140};
constexpr RGB warning{255, 190, 60};
constexpr RGB danger{255, 85, 100};
constexpr RGB gold{255, 210, 90};
constexpr RGB ink{14, 16, 26};        // dark text on bright badges
} // namespace palette

/// Width used for panels and tables: terminal width clamped to 64..104.
int layoutWidth();

// ── Text measurement & layout ───────────────────────────────────────────────
/// Printable width of a string, ignoring ANSI escapes and counting UTF-8
/// code points as one column each.
std::size_t visibleWidth(const std::string& s);
std::string pad(const std::string& s, std::size_t width, Align align = Align::Left);
/// Cut a plain (un-styled) UTF-8 string to `width` columns, adding "…".
std::string truncate(const std::string& s, std::size_t width);
std::string repeat(const std::string& s, std::size_t n);

// ── Colour ──────────────────────────────────────────────────────────────────
RGB lerp(RGB a, RGB b, double t);
std::string paint(const std::string& text, RGB color, bool bold = false);
std::string muted(const std::string& text);
std::string strong(const std::string& text);
/// Colour every character along a gradient from `from` to `to`.
std::string gradient(const std::string& text, RGB from, RGB to, bool bold = false);
/// Green → yellow → orange → red for a crowd level 0..100.
RGB crowdColor(double level);
/// A text label on a coloured background, e.g. "▐ FASTEST ▌".
std::string badge(const std::string& text, RGB background, RGB foreground = palette::ink);

// ── Meters ──────────────────────────────────────────────────────────────────
/// Horizontal bar with eighth-block precision and a colour gradient.
std::string meter(double fraction, int width, RGB from, RGB to);
/// Bar whose colour follows the crowd heat scale.
std::string crowdMeter(double level, int width);

// ── Blocks ──────────────────────────────────────────────────────────────────
void heading(const std::string& title, const std::string& subtitle = "");
void panel(const std::string& title, const std::vector<std::string>& lines,
           RGB border = palette::border, int width = 0);
void rule(RGB color = palette::border);

void success(const std::string& message);
void warn(const std::string& message);
void error(const std::string& message);
void info(const std::string& message);

/// Animated braille spinner shown for `durationMs` (skipped without animations).
void spinner(const std::string& message, int durationMs);

// ── Input ───────────────────────────────────────────────────────────────────
/// Line input with a styled prompt. nullopt on end-of-input.
std::optional<std::string> prompt(const std::string& label);
void pressEnter();

struct MenuItem {
    std::string label;
    std::string hint;
    RGB color = palette::accent;
};
/// Arrow-key menu (numbered list when not interactive).
/// Returns the chosen index, or -1 when the user backs out.
int menu(const std::string& title, const std::vector<MenuItem>& items, int initial = 0);

/// Redraw a block of lines in place (used by menus & autocomplete). Expects
/// the cursor on the block's first line and leaves it there.
void drawBlock(const std::vector<std::string>& lines);
/// Move the cursor from the first line of a drawn block to the line below it.
void endBlock(std::size_t lineCount);

// ── Tables ──────────────────────────────────────────────────────────────────
class Table {
public:
    void addColumn(const std::string& header, Align align = Align::Left);
    void addRow(const std::vector<std::string>& cells);
    void print(RGB accent = palette::accent) const;

private:
    std::vector<std::string> headers_;
    std::vector<Align> aligns_;
    std::vector<std::vector<std::string>> rows_;
};

} // namespace metro::ui
