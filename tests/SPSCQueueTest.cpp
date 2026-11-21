#include "SPSCQueue.hpp"

#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{

constexpr uint32_t QueueSize = 64;
constexpr uint32_t ControlQueueSize = 32;
constexpr uint64_t ThreadMessageCount = 1000000;
constexpr uint64_t RaisedRateMessageCount = 200000;
constexpr uint64_t ProcessMessageCount = 200000;

struct Payload
{
    uint64_t Value;
    uint64_t Check;
    uint8_t Bytes[48];
};

struct ControlMsg
{
    uint32_t MsgType;
    int32_t ChannelID;
    uint64_t MsgID;
};

constexpr size_t AlignUp(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

template <class T, uint32_t N>
constexpr size_t ExpectedQueueSize()
{
    return AlignUp(sizeof(T) * N, 128) + 256;
}

uint64_t Mix(uint64_t value)
{
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return value;
}

Payload MakePayload(uint64_t value)
{
    Payload payload{};
    payload.Value = value;
    payload.Check = Mix(value);
    for(size_t i = 0; i < sizeof(payload.Bytes); ++i)
    {
        payload.Bytes[i] = static_cast<uint8_t>(value + i);
    }
    return payload;
}

bool IsValidPayload(const Payload& payload, uint64_t expected)
{
    if(payload.Value != expected || payload.Check != Mix(expected))
    {
        return false;
    }
    for(size_t i = 0; i < sizeof(payload.Bytes); ++i)
    {
        if(payload.Bytes[i] != static_cast<uint8_t>(expected + i))
        {
            return false;
        }
    }
    return true;
}

void CpuRelax()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#else
    std::this_thread::yield();
#endif
}

bool Check(bool condition, const char* message)
{
    if(!condition)
    {
        std::fprintf(stderr, "FAIL: %s\n", message);
    }
    return condition;
}

bool TestBoundariesWrapAndReset()
{
    SHMIPC::SPSCQueue<Payload, 8> queue;
    Payload output{};

    for(uint64_t i = 0; i < 7; ++i)
    {
        if(!Check(queue.Push(MakePayload(i)), "SPSC push below capacity failed"))
        {
            return false;
        }
    }
    if(!Check(!queue.Push(MakePayload(7)),
              "SPSC accepted an element beyond N-1"))
    {
        return false;
    }
    for(uint64_t i = 0; i < 7; ++i)
    {
        if(!Check(queue.Pop(output), "SPSC pop failed") ||
           !Check(IsValidPayload(output, i),
                  "SPSC FIFO order or payload mismatch"))
        {
            return false;
        }
    }
    if(!Check(!queue.Pop(output), "SPSC pop on empty queue succeeded"))
    {
        return false;
    }

    for(uint64_t i = 0; i < 100000; ++i)
    {
        if(!Check(queue.Push(MakePayload(i)), "SPSC wrap push failed") ||
           !Check(queue.Pop(output), "SPSC wrap pop failed") ||
           !Check(IsValidPayload(output, i), "SPSC wrap payload mismatch"))
        {
            return false;
        }
    }

    for(uint64_t i = 0; i < 3; ++i)
    {
        if(!Check(queue.Push(MakePayload(i)), "SPSC pre-reset push failed"))
        {
            return false;
        }
    }
    if(!Check(queue.Pop(output), "SPSC pre-reset pop failed"))
    {
        return false;
    }

    queue.Reset();
    if(!Check(!queue.Pop(output), "SPSC reset did not restore empty state"))
    {
        return false;
    }
    for(uint64_t i = 0; i < 1000; ++i)
    {
        if(!Check(queue.Push(MakePayload(i)), "SPSC post-reset push failed") ||
           !Check(queue.Pop(output), "SPSC post-reset pop failed") ||
           !Check(IsValidPayload(output, i),
                  "SPSC post-reset payload mismatch"))
        {
            return false;
        }
    }
    return true;
}

bool TestQueueSpecializations()
{
    SHMIPC::SPSCQueue<ControlMsg, ControlQueueSize> controlQueue;
    SHMIPC::SPSCQueue<uint64_t, ControlQueueSize> dataQueue;
    ControlMsg control{};

    for(uint64_t i = 0; i + 1 < ControlQueueSize; ++i)
    {
        control.MsgType = 10;
        control.ChannelID = static_cast<int32_t>(i);
        control.MsgID = i;
        if(!controlQueue.Push(control) || !dataQueue.Push(i))
        {
            return Check(false, "specialized queue fill failed");
        }
    }
    if(!Check(!controlQueue.Push(control),
              "control queue accepted an element beyond N-1") ||
       !Check(!dataQueue.Push(0),
              "data queue accepted an element beyond N-1"))
    {
        return false;
    }

    for(uint64_t i = 0; i + 1 < ControlQueueSize; ++i)
    {
        uint64_t value = 0;
        if(!Check(controlQueue.Pop(control), "control queue pop failed") ||
           !Check(dataQueue.Pop(value), "data queue pop failed") ||
           !Check(control.ChannelID == static_cast<int32_t>(i) &&
                      control.MsgID == i,
                  "control queue FIFO mismatch") ||
           !Check(value == i, "data queue FIFO mismatch"))
        {
            return false;
        }
    }
    return true;
}

bool TestLayoutCompatibility()
{
    using SmallQueue = SHMIPC::SPSCQueue<Payload, 8>;
    using LargeQueue = SHMIPC::SPSCQueue<uint64_t, 8192>;
    return Check(sizeof(SmallQueue) == ExpectedQueueSize<Payload, 8>(),
                 "SPSC small queue size changed") &&
           Check(sizeof(LargeQueue) == ExpectedQueueSize<uint64_t, 8192>(),
                 "SPSC large queue size changed") &&
           Check(alignof(SmallQueue) == 128 &&
                     alignof(LargeQueue) == 128,
                 "SPSC queue alignment changed");
}

bool RunThreads(uint64_t messageCount, bool delayConsumer, bool delayProducer)
{
    SHMIPC::SPSCQueue<Payload, QueueSize> queue;
    std::atomic<bool> start{false};
    std::atomic<bool> producerDone{false};
    std::atomic<bool> valid{true};

    std::thread producer([&]() {
        while(!start.load(std::memory_order_acquire))
        {
            CpuRelax();
        }
        for(uint64_t i = 0; i < messageCount; ++i)
        {
            while(!queue.Push(MakePayload(i)))
            {
                CpuRelax();
            }
            if(delayProducer)
            {
                CpuRelax();
            }
        }
        producerDone.store(true, std::memory_order_release);
    });

    std::thread consumer([&]() {
        if(delayConsumer)
        {
            std::this_thread::sleep_for(std::chrono::microseconds(200));
        }
        while(!start.load(std::memory_order_acquire))
        {
            CpuRelax();
        }

        Payload payload{};
        uint64_t expected = 0;
        while(!producerDone.load(std::memory_order_acquire) ||
              expected < messageCount)
        {
            if(queue.Pop(payload))
            {
                if(expected >= messageCount ||
                   !IsValidPayload(payload, expected))
                {
                    valid.store(false, std::memory_order_relaxed);
                    return;
                }
                ++expected;
            }
            else
            {
                CpuRelax();
            }
        }
        if(expected != messageCount)
        {
            valid.store(false, std::memory_order_relaxed);
        }
    });

    start.store(true, std::memory_order_release);
    producer.join();
    consumer.join();
    return Check(valid.load(std::memory_order_relaxed),
                 "SPSC concurrent FIFO or payload validation failed");
}

bool TestThreadsAndRateImbalance()
{
    return RunThreads(ThreadMessageCount, false, false) &&
           RunThreads(RaisedRateMessageCount, true, false) &&
           RunThreads(RaisedRateMessageCount, false, true);
}

bool TestProcesses()
{
    using Queue = SHMIPC::SPSCQueue<Payload, QueueSize>;
    void* memory = mmap(nullptr, sizeof(Queue), PROT_READ | PROT_WRITE,
                        MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(!Check(memory != MAP_FAILED, "SPSC shared mmap failed"))
    {
        return false;
    }

    auto* queue = ::new(memory) Queue();
    const pid_t child = fork();
    if(child == 0)
    {
        for(uint64_t i = 0; i < ProcessMessageCount; ++i)
        {
            while(!queue->Push(MakePayload(i)))
            {
                sched_yield();
            }
        }
        _exit(0);
    }
    if(!Check(child > 0, "SPSC fork failed"))
    {
        queue->~Queue();
        munmap(memory, sizeof(Queue));
        return false;
    }

    bool valid = true;
    Payload payload{};
    for(uint64_t expected = 0; expected < ProcessMessageCount;)
    {
        if(queue->Pop(payload))
        {
            if(!IsValidPayload(payload, expected))
            {
                valid = false;
                break;
            }
            ++expected;
        }
        else
        {
            int status = 0;
            const pid_t result = waitpid(child, &status, WNOHANG);
            if(result == child)
            {
                valid = false;
                break;
            }
            sched_yield();
        }
    }

    int status = 0;
    waitpid(child, &status, 0);
    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        valid = false;
    }

    queue->~Queue();
    munmap(memory, sizeof(Queue));
    return Check(valid, "SPSC cross-process FIFO or payload validation failed");
}

} // namespace

int main(int argc, char* argv[])
{
    const bool skipProcessTest =
        argc > 1 && std::strcmp(argv[1], "--skip-fork") == 0;
    const auto start = std::chrono::steady_clock::now();

    const bool passed =
        TestBoundariesWrapAndReset() &&
        TestQueueSpecializations() &&
        TestLayoutCompatibility() &&
        TestThreadsAndRateImbalance() &&
        (skipProcessTest || TestProcesses());

    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    std::printf("SPSC queue tests %s in %lld ms\n",
                passed ? "passed" : "failed",
                static_cast<long long>(elapsed.count()));
    return passed ? 0 : 1;
}
