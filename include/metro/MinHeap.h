#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  MinHeap.h — a hand-written binary min-heap (priority queue).
//
//  Used by Dijkstra's algorithm (frontier of tentative distances) and by the
//  recommender (ranking candidate routes by score).
// ─────────────────────────────────────────────────────────────────────────────
#include <cstddef>
#include <functional>
#include <stdexcept>
#include <utility>
#include <vector>

namespace metro {

/// `Compare(a, b)` must return true when `a` should leave the heap before `b`.
template <typename T, typename Compare = std::less<T>>
class MinHeap {
public:
    explicit MinHeap(Compare cmp = Compare()) : cmp_(std::move(cmp)) {}

    /// Insert an element — O(log n).
    void push(T value) {
        data_.push_back(std::move(value));
        siftUp(data_.size() - 1);
    }

    /// Highest-priority element without removing it — O(1).
    const T& top() const {
        if (data_.empty()) throw std::out_of_range("MinHeap::top on empty heap");
        return data_.front();
    }

    /// Remove and return the highest-priority element — O(log n).
    T pop() {
        if (data_.empty()) throw std::out_of_range("MinHeap::pop on empty heap");
        T out = std::move(data_.front());
        if (data_.size() == 1) {
            data_.pop_back();
            return out;
        }
        data_.front() = std::move(data_.back());
        data_.pop_back();
        siftDown(0);
        return out;
    }

    bool empty() const { return data_.empty(); }
    std::size_t size() const { return data_.size(); }
    void clear() { data_.clear(); }

private:
    // Move element i up while it beats its parent.
    void siftUp(std::size_t i) {
        while (i > 0) {
            std::size_t parent = (i - 1) / 2;
            if (!cmp_(data_[i], data_[parent])) break;
            std::swap(data_[i], data_[parent]);
            i = parent;
        }
    }

    // Move element i down while a child beats it.
    void siftDown(std::size_t i) {
        const std::size_t n = data_.size();
        while (true) {
            std::size_t best = i;
            std::size_t left = 2 * i + 1;
            std::size_t right = left + 1;
            if (left < n && cmp_(data_[left], data_[best])) best = left;
            if (right < n && cmp_(data_[right], data_[best])) best = right;
            if (best == i) break;
            std::swap(data_[i], data_[best]);
            i = best;
        }
    }

    std::vector<T> data_;
    Compare cmp_;
};

} // namespace metro
