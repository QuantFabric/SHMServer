#ifndef SPSCQUEUE_HPP
#define SPSCQUEUE_HPP

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <mutex>
#include <atomic>

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

    SPSCQueue()
    {
        Reset();
    }

    void Reset()
    {
        m_Head.store(0, std::memory_order_relaxed);
        m_Tail.store(0, std::memory_order_relaxed);
        memset(m_Data, 0, sizeof(T) * N);
    }

    force_inline bool Push(const T& value)
    {
        const int WriteIndex = m_Tail.load(std::memory_order_relaxed);
        const int NextWriteIndex = (WriteIndex + 1) & (N - 1);
        // Queue is full
        if(NextWriteIndex == m_Head.load(std::memory_order_acquire))
        {
            return false;
        }
        // m_Data[WriteIndex] = value;
        memcpy(&m_Data[WriteIndex], &value, sizeof(T));
        // Update m_Tail
        m_Tail.store(NextWriteIndex, std::memory_order_release);
        return true;
    }

    force_inline bool Push(const T *value)
    {
        const int WriteIndex = m_Tail.load(std::memory_order_relaxed);
        const int NextWriteIndex = (WriteIndex + 1) & (N - 1);
        // Queue is full
        if(NextWriteIndex == m_Head.load(std::memory_order_acquire))
        {
            return false;
        }
        // m_Data[WriteIndex] = *value;
        memcpy(&m_Data[WriteIndex], value, sizeof(T));
        // Update m_Tail
        m_Tail.store(NextWriteIndex, std::memory_order_release);
        return true;
    }

    force_inline inline bool Pop(T &value)
    {
        const int ReadIndex = m_Head.load(std::memory_order_relaxed);
        // Queue is empty
        if (ReadIndex == m_Tail.load(std::memory_order_acquire))
        {
            return false;
        }
        // value = m_Data[ReadIndex];
        memcpy(&value, &m_Data[ReadIndex], sizeof(T));
        m_Head.store((ReadIndex + 1) & (N - 1), std::memory_order_release);
        return true;
    }
private:
    alignas(128) T m_Data[N] = {};          // 数据缓冲区
    alignas(128) std::atomic<int> m_Head;   // 队列头部索引
    alignas(128) std::atomic<int>  m_Tail;  // 队列尾部索引                            
};


}

#endif // SPSCQUEUE_HPP