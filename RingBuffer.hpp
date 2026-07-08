#ifndef RING_BUFFER_HPP
#define RING_BUFFER_HPP

#include <vector>
#include <mutex>
#include <condition_variable>
#include <algorithm>

class RingBuffer {
public:
    explicit RingBuffer(size_t size)
        : buffer(size), capacity(size), head(0), tail(0), count(0), closed(false) {}

    size_t write(const char* data, size_t len) {
        std::unique_lock<std::mutex> lock(mtx);
        size_t written = 0;

        while (written < len) {
            if (closed) break;

            not_full.wait(lock, [this] { return count < capacity || closed; });
            if (closed) break;

            size_t available = capacity - count;
            size_t to_write = std::min(len - written, available);

            // Space to the end of the buffer
            size_t to_end = capacity - tail;
            size_t chunk = std::min(to_write, to_end);

            std::copy(data + written, data + written + chunk, buffer.begin() + tail);

            tail = (tail + chunk) % capacity;
            count += chunk;
            written += chunk;

            not_empty.notify_one();
        }

        return written;
    }

    size_t read(char* data, size_t len) {
        std::unique_lock<std::mutex> lock(mtx);

        not_empty.wait(lock, [this] { return count > 0 || closed; });

        if (count == 0 && closed) return 0;

        size_t to_read = std::min(len, count);
        size_t read_count = 0;

        while (read_count < to_read) {
            size_t available_to_end = capacity - head;
            size_t chunk = std::min(to_read - read_count, available_to_end);

            std::copy(buffer.begin() + head, buffer.begin() + head + chunk, data + read_count);

            head = (head + chunk) % capacity;
            count -= chunk;
            read_count += chunk;

            not_full.notify_one();
        }

        return read_count;
    }

    void close() {
        {
            std::lock_guard<std::mutex> lock(mtx);
            closed = true;
        }
        not_full.notify_all();
        not_empty.notify_all();
    }

    bool is_closed() const {
        std::lock_guard<std::mutex> lock(mtx);
        return closed;
    }

private:
    std::vector<char> buffer;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
    bool closed;
    mutable std::mutex mtx;
    std::condition_variable not_full;
    std::condition_variable not_empty;
};

#endif // RING_BUFFER_HPP
