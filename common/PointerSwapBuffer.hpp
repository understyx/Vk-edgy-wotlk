#ifndef POINTER_SWAP_BUFFER_HPP
#define POINTER_SWAP_BUFFER_HPP

#include <atomic>
#include <memory>
#include <algorithm>

// Triple-buffering structure for lock-free SPSC data sharing
template <typename T>
class PointerSwapBuffer {
public:
    PointerSwapBuffer() {
        buffers[0] = std::make_unique<T>();
        buffers[1] = std::make_unique<T>();
        buffers[2] = std::make_unique<T>();

        producer_ptr = buffers[0].get();
        middle_ptr.store(buffers[1].get(), std::memory_order_relaxed);
        consumer_ptr = buffers[2].get();
        new_data_available.store(false, std::memory_order_relaxed);
    }

    // Producer writes to the returned pointer, then calls publish()
    T* get_write_buffer() {
        return producer_ptr;
    }

    void publish() {
        // Swap producer_ptr with middle_ptr
        T* old_middle = middle_ptr.exchange(producer_ptr, std::memory_order_acq_rel);
        producer_ptr = old_middle;
        new_data_available.store(true, std::memory_order_release);
    }

    // Consumer reads from the returned pointer, or checks if update is needed
    T* get_read_buffer() {
        return consumer_ptr;
    }

    bool update() {
        if (!new_data_available.load(std::memory_order_acquire)) {
            return false;
        }
        // Swap consumer_ptr with middle_ptr, reset before the swap
        new_data_available.store(false, std::memory_order_release);
        T* old_middle = middle_ptr.exchange(consumer_ptr, std::memory_order_acq_rel);
        consumer_ptr = old_middle;
        return true;
    }

private:
    std::unique_ptr<T> buffers[3];
    T* producer_ptr;
    std::atomic<T*> middle_ptr;
    T* consumer_ptr;
    std::atomic<bool> new_data_available;
};

// Double-buffering structure
template <typename T>
class DoubleBuffer {
public:
    DoubleBuffer() {
        buffers[0] = std::make_unique<T>();
        buffers[1] = std::make_unique<T>();
        front_idx.store(0, std::memory_order_relaxed);
    }

    T* get_front() {
        size_t idx = front_idx.load(std::memory_order_acquire);
        return buffers[idx].get();
    }

    T* get_back() {
        size_t idx = front_idx.load(std::memory_order_acquire);
        return buffers[1 - idx].get();
    }

    void swap() {
        size_t idx = front_idx.load(std::memory_order_relaxed);
        front_idx.store(1 - idx, std::memory_order_release);
    }

private:
    std::unique_ptr<T> buffers[2];
    std::atomic<size_t> front_idx;
};

#endif // POINTER_SWAP_BUFFER_HPP
