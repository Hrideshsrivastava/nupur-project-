#include "metro/Terminal.h"

#include <chrono>
#include <csignal>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <thread>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <conio.h>
#include <io.h>
#include <windows.h>
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>
#endif

namespace metro::term {

namespace {

ColorMode g_mode = ColorMode::TrueColor;
bool g_animations = true;

#ifndef _WIN32
termios g_savedTermios{};
#endif

bool stdoutIsTTY() {
#ifdef _WIN32
    return _isatty(_fileno(stdout)) != 0;
#else
    return isatty(STDOUT_FILENO) != 0;
#endif
}

bool stdinIsTTY() {
#ifdef _WIN32
    return _isatty(_fileno(stdin)) != 0;
#else
    return isatty(STDIN_FILENO) != 0;
#endif
}

#ifndef _WIN32
bool envContains(const char* name, const char* needle) {
    const char* v = std::getenv(name);
    return v && std::strstr(v, needle) != nullptr;
}
#endif

void onInterrupt(int) {
    // Restore colours and the cursor before leaving so the shell stays usable.
    const char msg[] = "\x1b[0m\x1b[?25h\n";
#ifdef _WIN32
    std::fwrite(msg, 1, sizeof msg - 1, stdout);
    std::fflush(stdout);
#else
    ssize_t ignored = write(STDOUT_FILENO, msg, sizeof msg - 1);
    (void)ignored;
#endif
    std::_Exit(130);
}

// Map a 24-bit colour onto the 6×6×6 cube of the xterm 256-colour palette.
int to256(RGB c) {
    auto level = [](std::uint8_t v) { return (v * 5 + 127) / 255; };
    return 16 + 36 * level(c.r) + 6 * level(c.g) + level(c.b);
}

} // namespace

void init() {
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    HANDLE out = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD mode = 0;
    bool vt = false;
    if (out != INVALID_HANDLE_VALUE && GetConsoleMode(out, &mode))
        vt = SetConsoleMode(out, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING) != 0;
    g_mode = vt ? ColorMode::TrueColor : ColorMode::None;
#else
    // Terminals that support 24-bit colour advertise it via COLORTERM; the
    // rest (e.g. macOS Terminal.app) get the closest 256-colour approximation.
    const bool truecolor = envContains("COLORTERM", "truecolor") || envContains("COLORTERM", "24bit");
    g_mode = truecolor ? ColorMode::TrueColor : ColorMode::Palette256;
    if (envContains("TERM", "dumb")) g_mode = ColorMode::None;
#endif
    if (std::getenv("NO_COLOR") || !stdoutIsTTY()) g_mode = ColorMode::None;
    if (!stdoutIsTTY()) g_animations = false;
    std::signal(SIGINT, onInterrupt);
}

ColorMode colorMode() { return g_mode; }
void setColorMode(ColorMode mode) { g_mode = mode; }

bool animations() { return g_animations; }
void setAnimations(bool enabled) { g_animations = enabled; }

bool interactive() { return stdinIsTTY() && stdoutIsTTY(); }

int columns() {
#ifdef _WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info))
        return info.srWindow.Right - info.srWindow.Left + 1;
#else
    winsize ws{};
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) return ws.ws_col;
#endif
    if (const char* c = std::getenv("COLUMNS")) {
        const int v = std::atoi(c);
        if (v > 0) return v;
    }
    return 100;
}

std::string fg(RGB c) {
    switch (g_mode) {
        case ColorMode::None: return "";
        case ColorMode::Palette256: return "\x1b[38;5;" + std::to_string(to256(c)) + "m";
        case ColorMode::TrueColor:
            return "\x1b[38;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" +
                   std::to_string(c.b) + "m";
    }
    return "";
}

std::string bg(RGB c) {
    switch (g_mode) {
        case ColorMode::None: return "";
        case ColorMode::Palette256: return "\x1b[48;5;" + std::to_string(to256(c)) + "m";
        case ColorMode::TrueColor:
            return "\x1b[48;2;" + std::to_string(c.r) + ";" + std::to_string(c.g) + ";" +
                   std::to_string(c.b) + "m";
    }
    return "";
}

std::string reset() { return g_mode == ColorMode::None ? "" : "\x1b[0m"; }
std::string bold() { return g_mode == ColorMode::None ? "" : "\x1b[1m"; }
std::string dim() { return g_mode == ColorMode::None ? "" : "\x1b[2m"; }
std::string italic() { return g_mode == ColorMode::None ? "" : "\x1b[3m"; }

void clearScreen() {
    if (!stdoutIsTTY()) return;
    std::cout << "\x1b[2J\x1b[H" << std::flush;
}

void hideCursor() {
    if (stdoutIsTTY()) std::cout << "\x1b[?25l" << std::flush;
}

void showCursor() {
    if (stdoutIsTTY()) std::cout << "\x1b[?25h" << std::flush;
}

void flush() { std::cout << std::flush; }

void sleepMs(int ms) {
    if (!g_animations || ms <= 0) return;
    std::cout << std::flush;
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

#ifdef _WIN32

RawMode::RawMode() : active_(true) {} // _getch() is already unbuffered
RawMode::~RawMode() = default;

int readKey() {
    const int c = _getch();
    if (c == 0 || c == 224) { // extended key: a second code follows
        switch (_getch()) {
            case 72: return KeyUp;
            case 80: return KeyDown;
            case 75: return KeyLeft;
            case 77: return KeyRight;
            default: return KeyNone;
        }
    }
    switch (c) {
        case 13: return KeyEnter;
        case 8: return KeyBackspace;
        case 9: return KeyTab;
        case 27: return KeyEscape;
        case 3: return KeyCtrlC;
        case 26: return KeyEOF;
        default: return c;
    }
}

#else

RawMode::RawMode() {
    if (!isatty(STDIN_FILENO)) return;
    if (tcgetattr(STDIN_FILENO, &g_savedTermios) != 0) return;
    termios raw = g_savedTermios;
    raw.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO | ISIG));
    raw.c_iflag &= static_cast<tcflag_t>(~(IXON | ICRNL));
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;
    active_ = tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == 0;
}

RawMode::~RawMode() {
    if (active_) tcsetattr(STDIN_FILENO, TCSAFLUSH, &g_savedTermios);
}

namespace {
// True if another byte arrives within `ms` milliseconds (escape sequences).
bool byteReady(int ms) {
    fd_set set;
    FD_ZERO(&set);
    FD_SET(STDIN_FILENO, &set);
    timeval tv{0, ms * 1000};
    return select(STDIN_FILENO + 1, &set, nullptr, nullptr, &tv) > 0;
}
} // namespace

int readKey() {
    std::cout << std::flush;
    unsigned char c = 0;
    if (read(STDIN_FILENO, &c, 1) != 1) return KeyEOF;
    if (c == 27) {
        if (!byteReady(40)) return KeyEscape;
        unsigned char seq[2] = {0, 0};
        if (read(STDIN_FILENO, &seq[0], 1) != 1) return KeyEscape;
        if (seq[0] != '[' && seq[0] != 'O') return KeyNone;
        if (read(STDIN_FILENO, &seq[1], 1) != 1) return KeyEscape;
        switch (seq[1]) {
            case 'A': return KeyUp;
            case 'B': return KeyDown;
            case 'C': return KeyRight;
            case 'D': return KeyLeft;
            default:
                // Swallow the rest of sequences like ESC [ 3 ~ (Delete).
                while (seq[1] >= '0' && seq[1] <= '9' && byteReady(10)) {
                    if (read(STDIN_FILENO, &seq[1], 1) != 1) break;
                }
                return KeyNone;
        }
    }
    switch (c) {
        case '\r':
        case '\n': return KeyEnter;
        case 127:
        case 8: return KeyBackspace;
        case '\t': return KeyTab;
        case 3: return KeyCtrlC;
        case 4: return KeyEOF;
        default: return c;
    }
}

#endif

} // namespace metro::term
