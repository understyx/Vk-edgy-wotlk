#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <atomic>
#include <thread>
#include <algorithm>

class RingBuffer {
public:
    explicit RingBuffer(size_t size)
        : buffer(size + 1), capacity(size + 1), head(0), tail(0), closed(false) {}

    // Writer side (Producer)
    size_t write(const char* data, size_t len) {
        size_t written = 0;
        while (written < len) {
            if (closed.load(std::memory_order_acquire)) break;

            size_t h = head.load(std::memory_order_acquire);
            size_t t = tail.load(std::memory_order_relaxed);

            size_t available = (h > t) ? (h - t - 1) : (capacity - (t - h) - 1);

            if (available == 0) {
                std::this_thread::yield();
                continue;
            }

            size_t to_write = std::min(len - written, available);
            size_t to_end = capacity - t;
            size_t chunk = std::min(to_write, to_end);

            std::copy(data + written, data + written + chunk, buffer.begin() + t);

            tail.store((t + chunk) % capacity, std::memory_order_release);
            written += chunk;
        }
        return written;
    }

    // Reader side (Consumer)
    size_t read_exactly(char* data, size_t len) {
        size_t read_count = 0;
        while (read_count < len) {
            size_t h = head.load(std::memory_order_relaxed);
            size_t t = tail.load(std::memory_order_acquire);

            size_t occupied = (t >= h) ? (t - h) : (capacity - (h - t));

            if (occupied == 0) {
                if (closed.load(std::memory_order_acquire)) break;
                std::this_thread::yield();
                continue;
            }

            // Wait for the full amount requested unless the buffer is closed
            if (occupied < (len - read_count) && !closed.load(std::memory_order_acquire)) {
                std::this_thread::yield();
                continue;
            }

            size_t to_read = std::min(len - read_count, occupied);
            size_t to_end = capacity - h;
            size_t chunk = std::min(to_read, to_end);

            std::copy(buffer.begin() + h, buffer.begin() + h + chunk, data + read_count);

            head.store((h + chunk) % capacity, std::memory_order_release);
            read_count += chunk;
        }
        return read_count;
    }

    // Allows checking data without consuming it
    size_t peek(char* data, size_t len) const {
        size_t h = head.load(std::memory_order_acquire);
        size_t t = tail.load(std::memory_order_acquire);

        size_t occupied = (t >= h) ? (t - h) : (capacity - (h - t));
        size_t to_read = std::min(len, occupied);
        size_t read_count = 0;
        size_t current_h = h;

        while (read_count < to_read) {
            size_t to_end = capacity - current_h;
            size_t chunk = std::min(to_read - read_count, to_end);
            std::copy(buffer.begin() + current_h, buffer.begin() + current_h + chunk, data + read_count);
            current_h = (current_h + chunk) % capacity;
            read_count += chunk;
        }
        return read_count;
    }

    size_t size() const {
        size_t h = head.load(std::memory_order_acquire);
        size_t t = tail.load(std::memory_order_acquire);
        if (t >= h) return t - h;
        return capacity - (h - t);
    }

    void close() {
        closed.store(true, std::memory_order_release);
    }

    bool is_closed() const {
        return closed.load(std::memory_order_acquire);
    }

private:
    std::vector<char> buffer;
    size_t capacity;
    std::atomic<size_t> head;
    std::atomic<size_t> tail;
    std::atomic<bool> closed;
};

#endif // RING_BUFFER_HPP
