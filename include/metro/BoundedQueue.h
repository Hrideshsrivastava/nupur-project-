#pragma once
// ─────────────────────────────────────────────────────────────────────────────
//  BoundedQueue.h — a fixed-capacity FIFO queue on a circular buffer.
//
//  Used for journey history: when the queue is full, enqueueing a new journey
//  evicts the oldest one (first in, first out).
// ─────────────────────────────────────────────────────────────────────────────
#include <cstddef>
#include <stdexcept>
#include <vector>

namespace metro {

template <typename T>
class BoundedQueue {
public:
    explicit BoundedQueue(std::size_t capacity) : buffer_(capacity == 0 ? 1 : capacity) {}

    /// Append to the back. Returns true when the oldest element was evicted.
    bool enqueue(const T& value) {
        bool evicted = false;
        if (full()) {
            head_ = (head_ + 1) % buffer_.size(); // drop the oldest
            --count_;
            evicted = true;
        }
        buffer_[(head_ + count_) % buffer_.size()] = value;
        ++count_;
        return evicted;
    }

    /// Remove and return the oldest element.
    T dequeue() {
        if (empty()) throw std::out_of_range("BoundedQueue::dequeue on empty queue");
        T value = buffer_[head_];
        head_ = (head_ + 1) % buffer_.size();
        --count_;
        return value;
    }

    const T& front() const { return at(0); }
    const T& back() const { return at(count_ - 1); }

    /// Element i positions after the oldest (0 = oldest, size()-1 = newest).
    const T& at(std::size_t i) const {
        if (i >= count_) throw std::out_of_range("BoundedQueue::at out of range");
        return buffer_[(head_ + i) % buffer_.size()];
    }

    std::size_t size() const { return count_; }
    std::size_t capacity() const { return buffer_.size(); }
    bool empty() const { return count_ == 0; }
    bool full() const { return count_ == buffer_.size(); }
    void clear() { head_ = 0; count_ = 0; }

private:
    std::vector<T> buffer_;
    std::size_t head_ = 0;  // index of the oldest element
    std::size_t count_ = 0; // number of stored elements
};

} // namespace metro
