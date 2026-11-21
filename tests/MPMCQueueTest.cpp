#include "MPMCQueue.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

namespace
{

constexpr size_t QueueSize = 64;
constexpr size_t SmallQueueSize = 8;
bool QuickMode = false;

struct Payload
{
    uint64_t Value;
    uint64_t Check;
    uint8_t Bytes[48];
};

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

bool TestCapacityBoundaries()
{
    SHMIPC::MPMCQueue<Payload, SmallQueueSize> queue;
    Payload output{};
    if(!Check(queue.Capacity() == SmallQueueSize,
              "MPMC capacity mismatch") ||
       !Check(queue.Size() == 0, "MPMC initial size mismatch"))
    {
        return false;
    }

    for(uint64_t i = 0; i < SmallQueueSize; ++i)
    {
        if(!Check(queue.Push(MakePayload(i)),
                  "MPMC push below capacity failed"))
        {
            return false;
        }
    }
    if(!Check(queue.Size() == SmallQueueSize,
              "MPMC size after filling mismatch") ||
       !Check(!queue.Push(MakePayload(SmallQueueSize)),
              "MPMC accepted more than N elements"))
    {
        return false;
    }

    for(uint64_t i = 0; i < SmallQueueSize; ++i)
    {
        if(!Check(queue.Pop(output), "MPMC pop failed") ||
           !Check(IsValidPayload(output, i),
                  "MPMC FIFO order or payload mismatch"))
        {
            return false;
        }
    }
    return Check(queue.Size() == 0, "MPMC final size mismatch") &&
           Check(!queue.Pop(output), "MPMC pop on empty queue succeeded");
}

bool TestWrapAndQueueIsolation()
{
    SHMIPC::MPMCQueue<Payload, SmallQueueSize> firstQueue;
    SHMIPC::MPMCQueue<Payload, SmallQueueSize> secondQueue;
    Payload output{};

    const uint64_t cycleCount = QuickMode ? 100 : 10000;
    for(uint64_t cycle = 0; cycle < cycleCount; ++cycle)
    {
        const uint64_t base = cycle * SmallQueueSize;
        for(uint64_t offset = 0; offset < SmallQueueSize; ++offset)
        {
            if(!Check(firstQueue.Push(MakePayload(base + offset)),
                      "MPMC wrap push failed") ||
               !Check(secondQueue.Push(
                          MakePayload(1000000 + base + offset)),
                      "MPMC isolated queue push failed"))
            {
                return false;
            }
        }
        for(uint64_t offset = 0; offset < SmallQueueSize; ++offset)
        {
            if(!Check(firstQueue.Pop(output), "MPMC wrap pop failed") ||
               !Check(IsValidPayload(output, base + offset),
                      "MPMC wrap payload mismatch") ||
               !Check(secondQueue.Pop(output),
                      "MPMC isolated queue pop failed") ||
               !Check(IsValidPayload(output,
                                     1000000 + base + offset),
                      "MPMC isolated queue payload mismatch"))
            {
                return false;
            }
        }
    }
    return true;
}

bool TestSingleElementQueue()
{
    SHMIPC::MPMCQueue<uint64_t, 1> queue;
    uint64_t output = 0;
    const uint64_t iterationCount = QuickMode ? 100 : 10000;
    for(uint64_t value = 0; value < iterationCount; ++value)
    {
        if(!Check(queue.Push(value), "MPMC N=1 push failed") ||
           !Check(!queue.Push(value + 1),
                  "MPMC N=1 accepted a second element") ||
           !Check(queue.Size() == 1, "MPMC N=1 size mismatch") ||
           !Check(queue.Pop(output), "MPMC N=1 pop failed") ||
           !Check(output == value, "MPMC N=1 value mismatch") ||
           !Check(queue.Size() == 0, "MPMC N=1 empty size mismatch") ||
           !Check(!queue.Pop(output),
                  "MPMC N=1 pop on empty queue succeeded"))
        {
            return false;
        }
    }
    return true;
}

bool RunConcurrent(size_t producerCount, size_t consumerCount,
                   size_t itemsPerProducer, bool slowProducers,
                   bool delayedConsumers)
{
    SHMIPC::MPMCQueue<Payload, QueueSize> queue;
    const size_t totalItems = producerCount * itemsPerProducer;
    std::atomic<size_t> ready{0};
    std::atomic<bool> start{false};
    std::atomic<size_t> producersDone{0};
    std::atomic<size_t> consumedItems{0};
    std::atomic<bool> valid{true};
    std::vector<std::vector<uint64_t>> consumedByThread(consumerCount);
    std::vector<std::thread> threads;
    threads.reserve(producerCount + consumerCount);

    for(size_t producer = 0; producer < producerCount; ++producer)
    {
        threads.emplace_back([&, producer]() {
            ready.fetch_add(1, std::memory_order_release);
            while(!start.load(std::memory_order_acquire))
            {
                CpuRelax();
            }

            const uint64_t begin = producer * itemsPerProducer;
            const uint64_t end = begin + itemsPerProducer;
            for(uint64_t value = begin; value < end; ++value)
            {
                while(!queue.Push(MakePayload(value)))
                {
                    CpuRelax();
                }
                if(slowProducers && (value - begin) % 32 == 0)
                {
                    std::this_thread::yield();
                }
            }
            producersDone.fetch_add(1, std::memory_order_release);
        });
    }

    for(size_t consumer = 0; consumer < consumerCount; ++consumer)
    {
        threads.emplace_back([&, consumer]() {
            ready.fetch_add(1, std::memory_order_release);
            while(!start.load(std::memory_order_acquire))
            {
                CpuRelax();
            }
            if(delayedConsumers)
            {
                std::this_thread::sleep_for(
                    std::chrono::microseconds(200));
            }

            auto& consumed = consumedByThread[consumer];
            consumed.reserve(totalItems);
            Payload payload{};
            auto idleDeadline =
                std::chrono::steady_clock::now() +
                std::chrono::seconds(QuickMode ? 300 : 10);

            while(consumedItems.load(std::memory_order_acquire) <
                  totalItems)
            {
                if(queue.Pop(payload))
                {
                    if(!IsValidPayload(payload, payload.Value))
                    {
                        valid.store(false, std::memory_order_relaxed);
                        return;
                    }
                    consumed.push_back(payload.Value);
                    consumedItems.fetch_add(1,
                                            std::memory_order_release);
                }
                else
                {
                    CpuRelax();
                    if(producersDone.load(std::memory_order_acquire) ==
                           producerCount &&
                       std::chrono::steady_clock::now() >= idleDeadline)
                    {
                        valid.store(false, std::memory_order_relaxed);
                        return;
                    }
                }
            }
        });
    }

    while(ready.load(std::memory_order_acquire) <
          producerCount + consumerCount)
    {
        CpuRelax();
    }
    start.store(true, std::memory_order_release);
    for(auto& thread : threads)
    {
        thread.join();
    }

    std::vector<uint64_t> allConsumed;
    allConsumed.reserve(totalItems);
    for(const auto& consumed : consumedByThread)
    {
        allConsumed.insert(allConsumed.end(), consumed.begin(),
                           consumed.end());
    }
    std::sort(allConsumed.begin(), allConsumed.end());

    if(!Check(allConsumed.size() == totalItems,
              "MPMC item count mismatch"))
    {
        return false;
    }
    for(size_t i = 0; i < allConsumed.size(); ++i)
    {
        if(allConsumed[i] != i)
        {
            return Check(false,
                         "MPMC item loss or duplication detected");
        }
    }
    return Check(valid.load(std::memory_order_relaxed) &&
                     queue.Size() == 0,
                 "MPMC concurrent validation or final size failed");
}

bool TestConcurrentTopologies()
{
    const size_t largeCount = QuickMode ? 200 : 500000;
    const size_t mediumCount = QuickMode ? 100 : 250000;
    const size_t smallCount = QuickMode ? 50 : 125000;
    const size_t imbalanceCount = QuickMode ? 20 : 100000;
    return RunConcurrent(1, 1, largeCount, false, false) &&
           RunConcurrent(2, 2, mediumCount, false, false) &&
           RunConcurrent(4, 4, smallCount, false, false) &&
           RunConcurrent(1, 1, imbalanceCount, true, false) &&
           RunConcurrent(1, 1, imbalanceCount, false, true);
}

} // namespace

int main(int argc, char* argv[])
{
    QuickMode = argc > 1 && std::strcmp(argv[1], "--quick") == 0;
    const auto start = std::chrono::steady_clock::now();
    const bool passed =
        TestCapacityBoundaries() &&
        TestWrapAndQueueIsolation() &&
        TestSingleElementQueue() &&
        TestConcurrentTopologies();
    const auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now() - start);
    std::printf("MPMC queue tests %s in %lld ms\n",
                passed ? "passed" : "failed",
                static_cast<long long>(elapsed.count()));
    return passed ? 0 : 1;
}
