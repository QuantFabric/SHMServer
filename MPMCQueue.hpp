#ifndef MPMCQUEUE_HPP
#define MPMCQUEUE_HPP

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
class MPMCQueue
{
public:
    static_assert(N && !(N & (N - 1)), "N must be a power of 2");
    explicit MPMCQueue()
    {
        m_QueueMask = N - 1;
        m_Data = (QueueNode*)new char[sizeof(QueueNode) * N];
        for(size_t i = 0; i < N; ++i)
        {
            m_Data[i].tail.store(i, std::memory_order_relaxed);
            m_Data[i].head.store(-1, std::memory_order_relaxed);
        }
        m_Tail.store(0, std::memory_order_relaxed);
        m_Head.store(0, std::memory_order_relaxed);
    }

    ~MPMCQueue()
    {
        for(size_t i = m_Head; i != m_Tail; ++i)
        {
            (&m_Data[i & m_QueueMask].data)->~T();
        }
        delete [] (char*)m_Data;
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
            node = &m_Data[tail & m_QueueMask];
            if(node->tail.load(std::memory_order_relaxed) != tail)
            {
                return false;
            }
            if((m_Tail.compare_exchange_weak(tail, tail + 1, std::memory_order_relaxed)))
                break;
        }
        new (&node->data)T(data);
        // Update 
        node->head.store(tail, std::memory_order_release);
        return true;
    }

    bool Pop(T& data)
    {
        QueueNode* node = NULL;
        size_t head = m_Head.load(std::memory_order_relaxed);
        for(;;)
        {
            node = &m_Data[head & m_QueueMask];
            if(node->head.load(std::memory_order_relaxed) != head)
            {
                return false;
            }
            if(m_Head.compare_exchange_weak(head, head + 1, std::memory_order_relaxed))
                break;
        }
        data = node->data;
        (&node->data)->~T();
        node->tail.store(head + N, std::memory_order_release);
        return true;
    }

protected:
    MPMCQueue(const MPMCQueue& other);
    MPMCQueue& operator=(const MPMCQueue&);
protected:
    struct alignas(64) QueueNode
    {
        T data;
        std::atomic<size_t> tail;
        std::atomic<size_t> head;
    };
    alignas(128) QueueNode* m_Data;
    alignas(128) size_t m_QueueMask;
    alignas(128) std::atomic<size_t> m_Tail;
    alignas(128) std::atomic<size_t> m_Head;
};

} // naespace SHMIPC


#endif // MPMCQUEUE_HPP