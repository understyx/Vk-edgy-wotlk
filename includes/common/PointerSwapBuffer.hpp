#pragma once
#include <atomic>
#include <memory>

template <typename T>
class PointerSwapBuffer {
public:
    PointerSwapBuffer() {
        m_buffers[0] = std::make_unique<T>();
        m_buffers[1] = std::make_unique<T>();
        m_buffers[2] = std::make_unique<T>();

        m_producerBuffer = m_buffers[0].get();
        m_consumerBuffer = m_buffers[1].get();
        m_sharedBuffer.store(m_buffers[2].get(), std::memory_order_relaxed);
        m_newUpdate.store(false, std::memory_order_relaxed);
    }

    // Non-copyable, non-movable
    PointerSwapBuffer(const PointerSwapBuffer&) = delete;
    PointerSwapBuffer& operator=(const PointerSwapBuffer&) = delete;

    // Producer writes to this pointer
    T* getWriteBuffer() {
        return m_producerBuffer;
    }

    // Producer submits the written data
    void swapProducer() {
        T* oldShared = m_sharedBuffer.exchange(m_producerBuffer, std::memory_order_acq_rel);
        m_producerBuffer = oldShared;
        m_newUpdate.store(true, std::memory_order_release);
    }

    // Consumer reads from this pointer
    T* getReadBuffer() {
        return m_consumerBuffer;
    }

    // Consumer retrieves the latest data if available. Returns true if swapped.
    // Atomically exchanges m_newUpdate to false to eliminate the state-loss race condition.
    bool swapConsumer() {
        if (!m_newUpdate.exchange(false, std::memory_order_acq_rel)) {
            return false;
        }
        T* oldShared = m_sharedBuffer.exchange(m_consumerBuffer, std::memory_order_acq_rel);
        m_consumerBuffer = oldShared;
        return true;
    }

private:
    std::unique_ptr<T> m_buffers[3];
    T* m_producerBuffer;
    T* m_consumerBuffer;
    std::atomic<T*> m_sharedBuffer;
    std::atomic<bool> m_newUpdate;
};
