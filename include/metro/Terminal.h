#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Terminal.h — the thin, cross-platform layer between the app and the
//  console: colour support detection, ANSI escape codes, raw keyboard input
//  and terminal size. Works on Linux, macOS and Windows 10+.
// ─────────────────────────────────────────────────────────────────────────────
#include "metro/Types.h"

#include <string>

namespace metro::term {

enum class ColorMode { None, Palette256, TrueColor };

/// Special key codes returned by readKey(); printable keys return their char.
enum Key : int {
    KeyNone = -1,
    KeyEnter = 1000,
    KeyBackspace,
    KeyTab,
    KeyEscape,
    KeyUp,
    KeyDown,
    KeyLeft,
    KeyRight,
    KeyCtrlC,
    KeyEOF,
};

/// Enable UTF-8 + ANSI processing (Windows), detect colour support and
/// install a Ctrl+C handler that restores the cursor.
void init();

ColorMode colorMode();
void setColorMode(ColorMode mode);

bool animations();
void setAnimations(bool enabled);

/// True when both stdin and stdout are attached to a real terminal, i.e.
/// arrow-key menus and live autocomplete can be used.
bool interactive();

/// Width of the terminal in columns (falls back to 100).
int columns();

// ── ANSI styling (all return "" when colours are disabled) ──────────────────
std::string fg(RGB color);
std::string bg(RGB color);
std::string reset();
std::string bold();
std::string dim();
std::string italic();

// ── Screen control ──────────────────────────────────────────────────────────
void clearScreen();
void hideCursor();
void showCursor();
void flush();

/// Sleep for an animation frame; does nothing when animations are off.
void sleepMs(int ms);

/// Puts the terminal into raw (unbuffered, no-echo) mode for its lifetime.
class RawMode {
public:
    RawMode();
    ~RawMode();
    RawMode(const RawMode&) = delete;
    RawMode& operator=(const RawMode&) = delete;

private:
    bool active_ = false;
};

/// Block until one key is pressed (call while a RawMode object is alive).
int readKey();

} // namespace metro::term
