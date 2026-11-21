#ifndef MPMCQUEUE_HPP
#define MPMCQUEUE_HPP

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <new>
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
class MPMCQueue
{
public:
    static_assert(N && !(N & (N - 1)), "N must be a power of 2");
    static_assert(std::is_trivially_copyable<T>::value,
                  "T must be trivially copyable");
    static_assert(std::is_trivially_copy_constructible<T>::value,
                  "T must be trivially copy constructible");
    static_assert(std::is_trivially_copy_assignable<T>::value,
                  "T must be trivially copy assignable");
    static_assert(std::is_trivially_destructible<T>::value,
                  "T must be trivially destructible");
    static_assert(std::atomic<size_t>::is_always_lock_free,
                  "std::atomic<size_t> must be lock-free");

    explicit MPMCQueue()
    {
        m_Data = new QueueNode[N];
        for(size_t i = 0; i < N; ++i)
        {
            m_Data[i].sequence.store(FreeSequence(i), std::memory_order_relaxed);
        }
        m_Tail.store(0, std::memory_order_relaxed);
        m_Head.store(0, std::memory_order_relaxed);
    }

    ~MPMCQueue()
    {
        delete[] m_Data;
    }

    size_t Capacity() const
    {
        return N;
    }

    size_t Size() const
    {
        size_t head = m_Head.load(std::memory_order_acquire);
        size_t tail = m_Tail.load(std::memory_order_relaxed);
        return tail - head;
    }

    bool Push(const T& data)
    {
        QueueNode* node = NULL;
        size_t tail = m_Tail.load(std::memory_order_relaxed);
        for(;;)
        {
            node = &m_Data[tail & (N - 1)];
            if(node->sequence.load(std::memory_order_acquire) != FreeSequence(tail))
            {
                return false;
            }
            if(m_Tail.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed))
            {
                break;
            }
        }
        new(&node->data) T(data);
        node->sequence.store(ReadySequence(tail), std::memory_order_release);
        return true;
    }

    bool Pop(T& data)
    {
        QueueNode* node = NULL;
        size_t head = m_Head.load(std::memory_order_relaxed);
        for(;;)
        {
            node = &m_Data[head & (N - 1)];
            if(node->sequence.load(std::memory_order_acquire) != ReadySequence(head))
            {
                return false;
            }
            if(m_Head.compare_exchange_weak(head, head + 1, std::memory_order_relaxed))
            {
                break;
            }
        }
        T* slot = std::launder(reinterpret_cast<T*>(&node->data));
        data = *slot;
        slot->~T();
        node->sequence.store(FreeSequence(head + N), std::memory_order_release);
        return true;
    }

protected:
    MPMCQueue(const MPMCQueue& other);
    MPMCQueue& operator=(const MPMCQueue&);
protected:
    static constexpr size_t FreeSequence(size_t index)
    {
        if constexpr (N == 1)
        {
            return index << 1;
        }
        else
        {
            return index;
        }
    }

    static constexpr size_t ReadySequence(size_t index)
    {
        if constexpr (N == 1)
        {
            return (index << 1) | 1;
        }
        else
        {
            return index + 1;
        }
    }

    struct alignas(64) QueueNode
    {
        std::aligned_storage_t<sizeof(T), alignof(T)> data;
        std::atomic<size_t> sequence;
    };
    alignas(128) QueueNode* m_Data;
    alignas(128) std::atomic<size_t> m_Tail;
    alignas(128) std::atomic<size_t> m_Head;
};

} // namespace SHMIPC


#endif // MPMCQUEUE_HPP
