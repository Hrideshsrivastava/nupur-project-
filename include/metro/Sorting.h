#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  Sorting.h — a generic, stable, top-down merge sort (O(n log n)).
//
//  Used to order stations alphabetically, by crowd level, lines by length and
//  route options by any column.
// ─────────────────────────────────────────────────────────────────────────────
#include <cstddef>
#include <vector>

namespace metro {

namespace detail {

template <typename T, typename Compare>
void mergeSortRange(std::vector<T>& v, std::vector<T>& buf, std::size_t lo, std::size_t hi,
                    Compare& cmp) {
    if (hi - lo < 2) return;
    const std::size_t mid = lo + (hi - lo) / 2;
    mergeSortRange(v, buf, lo, mid, cmp);
    mergeSortRange(v, buf, mid, hi, cmp);

    // Merge the two sorted halves into buf, then copy back.
    std::size_t i = lo, j = mid, k = lo;
    while (i < mid && j < hi) {
        // Take from the right half only when strictly smaller => stable.
        if (cmp(v[j], v[i])) buf[k++] = v[j++];
        else                 buf[k++] = v[i++];
    }
    while (i < mid) buf[k++] = v[i++];
    while (j < hi) buf[k++] = v[j++];
    for (std::size_t x = lo; x < hi; ++x) v[x] = buf[x];
}

} // namespace detail

/// Sort `v` in place. `cmp(a, b)` returns true when a must come before b.
template <typename T, typename Compare>
void mergeSort(std::vector<T>& v, Compare cmp) {
    if (v.size() < 2) return;
    std::vector<T> buffer(v);
    detail::mergeSortRange(v, buffer, 0, v.size(), cmp);
}

} // namespace metro
