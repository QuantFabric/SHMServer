#ifndef SPSCQUEUE_HPP
#define SPSCQUEUE_HPP

#include <atomic>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>

#ifndef force_inline
#define force_inline __attribute__ ((__always_inline__))
#endif

// Branch Prediction
#ifdef __GNUC__

#ifndef likely
#define likely(x)    __builtin_expect(!!(x), 1)
#endif

#ifndef unlikely
#define unlikely(x)  __builtin_expect(!!(x), 0)
#endif

#else

#ifndef likely
#define likely(x)    (x)
#endif

#ifndef unlikely
#define unlikely(x)  (x)
#endif

#endif

namespace SHMIPC
{

template <class T, uint32_t N>
class SPSCQueue
{
public:
    static_assert(N && !(N & (N - 1)), "N must be a power of 2");
    static_assert(std::is_trivially_copyable<T>::value,
                  "T must be trivially copyable");
    static_assert(std::atomic<uint32_t>::is_always_lock_free,
                  "std::atomic<uint32_t> must be lock-free for shared memory");
    static_assert(sizeof(std::atomic<uint32_t>) + sizeof(uint32_t) <= 128,
                  "cursor cache must fit in the existing cache line");

    SPSCQueue()
    {
        Reset();
    }

    void Reset()
    {
        // Reset is not concurrent: producer and consumer must both be idle.
        m_Head.store(0, std::memory_order_relaxed);
        m_CachedTail = 0;
        m_Tail.store(0, std::memory_order_relaxed);
        m_CachedHead = 0;
    }

    force_inline bool Push(const T& value)
    {
        const uint32_t writeIndex = m_Tail.load(std::memory_order_relaxed);
        const uint32_t nextWriteIndex = (writeIndex + 1) & (N - 1);

        // Reload the consumer index only when the cached value suggests a full
        // queue. The successful path avoids touching the other endpoint's
        // cache line.
        if(nextWriteIndex == m_CachedHead)
        {
            m_CachedHead = m_Head.load(std::memory_order_acquire);
            if(nextWriteIndex == m_CachedHead)
            {
                return false;
            }
        }

        std::memcpy(&m_Data[writeIndex], &value, sizeof(T));
        m_Tail.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    force_inline bool Pop(T &value)
    {
        const uint32_t readIndex = m_Head.load(std::memory_order_relaxed);

        // Reload the producer index only when the cached value suggests an
        // empty queue.
        if(readIndex == m_CachedTail)
        {
            m_CachedTail = m_Tail.load(std::memory_order_acquire);
            if(readIndex == m_CachedTail)
            {
                return false;
            }
        }

        std::memcpy(&value, &m_Data[readIndex], sizeof(T));
        m_Head.store((readIndex + 1) & (N - 1), std::memory_order_release);
        return true;
    }

private:
    alignas(128) T m_Data[N];
    // m_Head and m_CachedTail share the consumer-owned cache line.
    alignas(128) std::atomic<uint32_t> m_Head;
    uint32_t m_CachedTail;
    // m_Tail and m_CachedHead share the producer-owned cache line.
    alignas(128) std::atomic<uint32_t> m_Tail;
    uint32_t m_CachedHead;
};


}

#endif // SPSCQUEUE_HPP
