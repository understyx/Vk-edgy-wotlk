#pragma once

#include <mutex>
#include <atomic>
#include <utility>

template <typename T>
class DoubleBuffer {
private:
    T* m_front;
    T* m_back;
    mutable std::mutex m_mutex;
    mutable std::atomic<bool> m_hasNewData{false};

public:
    DoubleBuffer() {
        m_front = new T();
        m_back = new T();
    }

    ~DoubleBuffer() {
        delete m_front;
        delete m_back;
    }

    // Disable copy/move
    DoubleBuffer(const DoubleBuffer&) = delete;
    DoubleBuffer& operator=(const DoubleBuffer&) = delete;

    // Get the back buffer for writing (Producer)
    T* getBack() {
        // Back buffer is owned by the producer, no lock needed to write to it.
        return m_back;
    }

    // Atomic swap (Producer calls this when done writing)
    void swap() {
        std::lock_guard<std::mutex> lock(m_mutex);
        std::swap(m_front, m_back);
        m_hasNewData.store(true, std::memory_order_release);
    }

    // Check if new data is available (Consumer)
    bool hasNewData() const {
        return m_hasNewData.load(std::memory_order_acquire);
    }

    // Get the front buffer for reading (Consumer)
    void getFrontCopy(T& dest) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        dest = *m_front;
        m_hasNewData.store(false, std::memory_order_release);
    }

    // Direct locked access callback
    template <typename Func>
    void withFront(Func&& func) const {
        std::lock_guard<std::mutex> lock(m_mutex);
        func(*m_front);
        m_hasNewData.store(false, std::memory_order_release);
    }
};
