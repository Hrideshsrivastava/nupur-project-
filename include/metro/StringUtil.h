#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  StringUtil.h — text helpers: normalisation for hashing/Trie, edit distance
//  for "did you mean…?", and clock/time formatting.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstddef>
#include <string>
#include <vector>

namespace metro::str {

std::string toLower(std::string s);
std::string trim(const std::string& s);

/// Hash-map key: lowercase letters and digits only. "Dilli Haat - INA" -> "dillihaatina".
std::string normalizeKey(const std::string& s);

/// Trie key: lowercase words separated by single spaces. "Dilli Haat - INA" -> "dilli haat ina".
std::string normalizeWords(const std::string& s);

std::vector<std::string> split(const std::string& s, char delimiter);

/// Levenshtein edit distance (insert / delete / substitute = 1).
std::size_t levenshtein(const std::string& a, const std::string& b);

/// 42 -> "42 min", 75 -> "1 h 15 min".
std::string formatDuration(double minutes);

/// Minute of day -> "HH:MM" (wraps past midnight).
std::string formatClock(double minuteOfDay);

/// Parses "9", "09:30", "9.30", "930", "1830". Returns false on bad input.
bool parseClock(const std::string& text, int& minuteOfDay);

/// Fixed-point number formatting, e.g. fixed(3.14159, 1) -> "3.1".
std::string fixed(double value, int precision);

} // namespace metro::str
