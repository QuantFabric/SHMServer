#include "MPMCQueue.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <thread>
#include <vector>

#include <pthread.h>
#include <sched.h>

namespace
{

constexpr uint32_t BenchmarkQueueSize = 1024;
constexpr uint32_t DefaultRounds = 7;

struct Options
{
    uint32_t Rounds = DefaultRounds;
};

struct Topology
{
    size_t Producers;
    size_t Consumers;
    uint64_t ItemsPerProducer;
    const char* Name;
};

struct RunResult
{
    double ThroughputMpps = 0;
    uint64_t P50Ns = 0;
    uint64_t P99Ns = 0;
    bool Valid = false;
};

template <size_t PayloadSize>
struct Payload
{
    static_assert(PayloadSize >= 16 && PayloadSize % 8 == 0,
                  "payload size must include two 64-bit fields");

    uint64_t Value;
    uint64_t TimeStamp;
    uint8_t Bytes[PayloadSize - 16];
};

// Snapshot of the original two-state-node algorithm with only the allocation
// alignment and acquire/release ordering fixed. This keeps the A/B comparison
// focused on the single-sequence node representation.
template <class T, uint32_t N>
class AlignedTwoStateQueue
{
public:
    static_assert(N && !(N & (N - 1)), "N must be a power of 2");

    AlignedTwoStateQueue()
        : m_QueueMask(N - 1)
    {
        m_Data = new QueueNode[N];
        for(size_t i = 0; i < N; ++i)
        {
            m_Data[i].free.store(i, std::memory_order_relaxed);
            m_Data[i].ready.store(static_cast<size_t>(-1),
                                  std::memory_order_relaxed);
        }
        m_Tail.store(0, std::memory_order_relaxed);
        m_Head.store(0, std::memory_order_relaxed);
    }

    ~AlignedTwoStateQueue()
    {
        delete[] m_Data;
    }

    size_t Size() const
    {
        const size_t head = m_Head.load(std::memory_order_acquire);
        const size_t tail = m_Tail.load(std::memory_order_relaxed);
        return tail - head;
    }

    bool Push(const T& data)
    {
        QueueNode* node = nullptr;
        size_t tail = m_Tail.load(std::memory_order_relaxed);
        for(;;)
        {
            node = &m_Data[tail & m_QueueMask];
            if(node->free.load(std::memory_order_acquire) != tail)
            {
                return false;
            }
            if(m_Tail.compare_exchange_weak(
                   tail, tail + 1, std::memory_order_relaxed))
            {
                break;
            }
        }
        ::new(static_cast<void*>(&node->data)) T(data);
        node->ready.store(tail, std::memory_order_release);
        return true;
    }

    bool Pop(T& data)
    {
        QueueNode* node = nullptr;
        size_t head = m_Head.load(std::memory_order_relaxed);
        for(;;)
        {
            node = &m_Data[head & m_QueueMask];
            if(node->ready.load(std::memory_order_acquire) != head)
            {
                return false;
            }
            if(m_Head.compare_exchange_weak(
                   head, head + 1, std::memory_order_relaxed))
            {
                break;
            }
        }
        T* slot = std::launder(reinterpret_cast<T*>(&node->data));
        data = *slot;
        slot->~T();
        node->free.store(head + N, std::memory_order_release);
        return true;
    }

private:
    struct alignas(64) QueueNode
    {
        std::aligned_storage_t<sizeof(T), alignof(T)> data;
        std::atomic<size_t> free;
        std::atomic<size_t> ready;
    };

    alignas(128) QueueNode* m_Data;
    alignas(128) size_t m_QueueMask;
    alignas(128) std::atomic<size_t> m_Tail;
    alignas(128) std::atomic<size_t> m_Head;
};

uint64_t NowNs()
{
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

void CpuRelax()
{
#if defined(__x86_64__) || defined(__i386__)
    __builtin_ia32_pause();
#else
    std::this_thread::yield();
#endif
}

int ReadTopologyValue(int cpu, const char* name)
{
    std::ifstream input(std::string("/sys/devices/system/cpu/cpu") +
                        std::to_string(cpu) + "/topology/" + name);
    int value = -1;
    input >> value;
    return value;
}

std::vector<int> GetAffinityCpus()
{
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    std::vector<int> cpus;
    if(sched_getaffinity(0, sizeof(cpuSet), &cpuSet) != 0)
    {
        return cpus;
    }
    for(int cpu = 0; cpu < CPU_SETSIZE; ++cpu)
    {
        if(CPU_ISSET(cpu, &cpuSet))
        {
            cpus.push_back(cpu);
        }
    }
    return cpus;
}

std::vector<int> SelectCpus(size_t count)
{
    const std::vector<int> allowed = GetAffinityCpus();
    std::vector<int> selected;
    if(allowed.empty() || count == 0)
    {
        return selected;
    }

    const int package = ReadTopologyValue(allowed.front(),
                                          "physical_package_id");
    std::vector<int> remaining;
    std::vector<int> seenCores;
    for(const int cpu : allowed)
    {
        if(ReadTopologyValue(cpu, "physical_package_id") != package)
        {
            remaining.push_back(cpu);
            continue;
        }
        const int core = ReadTopologyValue(cpu, "core_id");
        if(std::find(seenCores.begin(), seenCores.end(), core) ==
               seenCores.end() &&
           selected.size() < count)
        {
            seenCores.push_back(core);
            selected.push_back(cpu);
        }
        else
        {
            remaining.push_back(cpu);
        }
    }
    for(const int cpu : remaining)
    {
        if(selected.size() == count)
        {
            break;
        }
        selected.push_back(cpu);
    }
    return selected;
}

void PinCurrentThread(const std::vector<int>& cpus, size_t index)
{
    if(cpus.empty())
    {
        return;
    }
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpus[index % cpus.size()], &cpuSet);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet);
}

template <size_t PayloadSize>
Payload<PayloadSize> MakePayload(uint64_t value)
{
    Payload<PayloadSize> payload{};
    payload.Value = value;
    for(size_t i = 0; i < sizeof(payload.Bytes); ++i)
    {
        payload.Bytes[i] = static_cast<uint8_t>(value + i);
    }
    payload.TimeStamp = NowNs();
    return payload;
}

template <size_t PayloadSize>
bool IsValidPayload(const Payload<PayloadSize>& payload)
{
    for(size_t i = 0; i < sizeof(payload.Bytes); ++i)
    {
        if(payload.Bytes[i] != static_cast<uint8_t>(payload.Value + i))
        {
            return false;
        }
    }
    return true;
}

RunResult Summarize(const std::vector<uint64_t>& latencies,
                    uint64_t begin, uint64_t end, bool valid)
{
    RunResult result;
    result.Valid = valid;
    if(latencies.empty())
    {
        result.Valid = false;
        return result;
    }

    std::vector<uint64_t> sorted = latencies;
    std::sort(sorted.begin(), sorted.end());
    result.P50Ns = sorted[sorted.size() * 50 / 100];
    result.P99Ns = sorted[sorted.size() * 99 / 100];
    const double seconds = static_cast<double>(end - begin) / 1e9;
    result.ThroughputMpps =
        static_cast<double>(latencies.size()) / seconds / 1000000.0;
    return result;
}

template <class Queue, size_t PayloadSize>
RunResult RunSameProcess(const Topology& topology, const Options& options,
                         const std::vector<int>& cpus)
{
    using Message = Payload<PayloadSize>;
    Queue queue;
    const size_t totalItems =
        topology.Producers * topology.ItemsPerProducer;
    std::atomic<size_t> ready{0};
    std::atomic<bool> start{false};
    std::atomic<size_t> producersDone{0};
    std::atomic<size_t> consumedItems{0};
    std::atomic<bool> valid{true};
    std::vector<std::vector<uint64_t>> values(topology.Consumers);
    std::vector<std::vector<uint64_t>> latencies(topology.Consumers);
    std::vector<std::thread> threads;
    threads.reserve(topology.Producers + topology.Consumers);

    for(size_t producer = 0; producer < topology.Producers; ++producer)
    {
        threads.emplace_back([&, producer]() {
            PinCurrentThread(cpus, producer);
            ready.fetch_add(1, std::memory_order_release);
            while(!start.load(std::memory_order_acquire))
            {
                CpuRelax();
            }

            const uint64_t begin =
                producer * topology.ItemsPerProducer;
            const uint64_t end = begin + topology.ItemsPerProducer;
            for(uint64_t value = begin; value < end; ++value)
            {
                const Message message = MakePayload<PayloadSize>(value);
                while(!queue.Push(message))
                {
                    CpuRelax();
                }
            }
            producersDone.fetch_add(1, std::memory_order_release);
        });
    }

    for(size_t consumer = 0; consumer < topology.Consumers; ++consumer)
    {
        threads.emplace_back([&, consumer]() {
            PinCurrentThread(cpus, topology.Producers + consumer);
            ready.fetch_add(1, std::memory_order_release);
            while(!start.load(std::memory_order_acquire))
            {
                CpuRelax();
            }

            auto& consumed = values[consumer];
            auto& latency = latencies[consumer];
            consumed.reserve(totalItems);
            latency.reserve(totalItems);
            const uint64_t deadline = NowNs() + 30000000000ULL;
            Message message{};

            while(consumedItems.load(std::memory_order_acquire) <
                  totalItems)
            {
                if(queue.Pop(message))
                {
                    if(!IsValidPayload<PayloadSize>(message))
                    {
                        valid.store(false, std::memory_order_relaxed);
                    }
                    consumed.push_back(message.Value);
                    latency.push_back(NowNs() - message.TimeStamp);
                    consumedItems.fetch_add(1,
                                            std::memory_order_release);
                }
                else
                {
                    CpuRelax();
                    if(producersDone.load(std::memory_order_acquire) ==
                           topology.Producers &&
                       NowNs() > deadline)
                    {
                        valid.store(false, std::memory_order_relaxed);
                        break;
                    }
                }
            }
        });
    }

    while(ready.load(std::memory_order_acquire) <
          topology.Producers + topology.Consumers)
    {
        CpuRelax();
    }
    const uint64_t begin = NowNs();
    start.store(true, std::memory_order_release);
    for(auto& thread : threads)
    {
        thread.join();
    }
    const uint64_t end = NowNs();

    std::vector<uint64_t> allValues;
    std::vector<uint64_t> allLatencies;
    allValues.reserve(totalItems);
    allLatencies.reserve(totalItems);
    for(size_t consumer = 0; consumer < topology.Consumers; ++consumer)
    {
        allValues.insert(allValues.end(), values[consumer].begin(),
                         values[consumer].end());
        allLatencies.insert(allLatencies.end(),
                            latencies[consumer].begin(),
                            latencies[consumer].end());
    }
    std::sort(allValues.begin(), allValues.end());

    bool complete = allValues.size() == totalItems;
    for(size_t i = 0; complete && i < allValues.size(); ++i)
    {
        complete = allValues[i] == i;
    }
    valid.store(valid.load(std::memory_order_relaxed) && complete,
                std::memory_order_relaxed);

    RunResult result = Summarize(
        allLatencies, begin, end,
        valid.load(std::memory_order_relaxed) &&
            queue.Size() == 0);
    (void)options;
    return result;
}

double Median(std::vector<double> values)
{
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

uint64_t Median(std::vector<uint64_t> values)
{
    std::sort(values.begin(), values.end());
    return values[values.size() / 2];
}

bool AddResult(const RunResult& result, std::vector<double>& throughput,
               std::vector<uint64_t>& p50, std::vector<uint64_t>& p99)
{
    throughput.push_back(result.ThroughputMpps);
    p50.push_back(result.P50Ns);
    p99.push_back(result.P99Ns);
    return result.Valid;
}

void PrintSummary(const char* label,
                  const std::vector<double>& baselineThroughput,
                  const std::vector<uint64_t>& baselineP50,
                  const std::vector<uint64_t>& baselineP99,
                  const std::vector<double>& candidateThroughput,
                  const std::vector<uint64_t>& candidateP50,
                  const std::vector<uint64_t>& candidateP99)
{
    const double baselineMpps = Median(baselineThroughput);
    const double candidateMpps = Median(candidateThroughput);
    const double delta =
        baselineMpps == 0
            ? 0
            : (candidateMpps / baselineMpps - 1.0) * 100.0;
    std::printf(
        "%-12s baseline %.2f Mmsg/s p50=%lu p99=%lu | "
        "sequence %.2f Mmsg/s p50=%lu p99=%lu | throughput %+.1f%%\n",
        label, baselineMpps,
        static_cast<unsigned long>(Median(baselineP50)),
        static_cast<unsigned long>(Median(baselineP99)), candidateMpps,
        static_cast<unsigned long>(Median(candidateP50)),
        static_cast<unsigned long>(Median(candidateP99)), delta);
}

template <size_t PayloadSize>
bool RunPayloadSuite(const Topology& topology, const Options& options,
                     const std::vector<int>& cpus)
{
    using Message = Payload<PayloadSize>;
    using Baseline = AlignedTwoStateQueue<Message, BenchmarkQueueSize>;
    using Candidate = SHMIPC::MPMCQueue<Message, BenchmarkQueueSize>;

    const uint64_t warmupItems =
        std::min<uint64_t>(20000, topology.ItemsPerProducer);
    Topology warmup = topology;
    warmup.ItemsPerProducer = warmupItems;
    RunSameProcess<Baseline, PayloadSize>(warmup, options, cpus);
    RunSameProcess<Candidate, PayloadSize>(warmup, options, cpus);

    std::vector<double> baselineThroughput;
    std::vector<uint64_t> baselineP50;
    std::vector<uint64_t> baselineP99;
    std::vector<double> candidateThroughput;
    std::vector<uint64_t> candidateP50;
    std::vector<uint64_t> candidateP99;
    bool valid = true;

    for(uint32_t round = 0; round < options.Rounds; ++round)
    {
        RunResult baseline;
        RunResult candidate;
        if(round % 2 == 0)
        {
            baseline = RunSameProcess<Baseline, PayloadSize>(
                topology, options, cpus);
            candidate = RunSameProcess<Candidate, PayloadSize>(
                topology, options, cpus);
        }
        else
        {
            candidate = RunSameProcess<Candidate, PayloadSize>(
                topology, options, cpus);
            baseline = RunSameProcess<Baseline, PayloadSize>(
                topology, options, cpus);
        }
        valid = AddResult(baseline, baselineThroughput, baselineP50,
                          baselineP99) &&
                valid;
        valid = AddResult(candidate, candidateThroughput, candidateP50,
                          candidateP99) &&
                valid;
    }

    char label[32];
    std::snprintf(label, sizeof(label), "%zuB %s", PayloadSize,
                  topology.Name);
    PrintSummary(label, baselineThroughput, baselineP50, baselineP99,
                 candidateThroughput, candidateP50, candidateP99);
    return valid;
}

bool ParseOptions(int argc, char* argv[], Options& options)
{
    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "--runs") == 0 && i + 1 < argc)
        {
            options.Rounds =
                static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        }
        else
        {
            std::fprintf(stderr, "Usage: %s [--runs N]\n", argv[0]);
            return false;
        }
    }
    return options.Rounds > 0;
}

} // namespace

int main(int argc, char* argv[])
{
    Options options;
    if(!ParseOptions(argc, argv, options))
    {
        return 1;
    }

    const Topology topologies[] = {
        {1, 1, 500000, "1P1C"},
        {2, 2, 250000, "2P2C"},
        {4, 4, 125000, "4P4C"},
    };
    const std::vector<int> cpus = SelectCpus(8);
    std::printf("MPMC A/B benchmark: %u rounds, CPUs:", options.Rounds);
    for(const int cpu : cpus)
    {
        std::printf(" %d", cpu);
    }
    std::printf("\n");

    bool valid = true;
    for(const Topology& topology : topologies)
    {
        valid = RunPayloadSuite<64>(topology, options, cpus) && valid;
        valid = RunPayloadSuite<1024>(topology, options, cpus) && valid;
    }
    if(!valid)
    {
        std::fprintf(stderr, "MPMC benchmark validation failed\n");
        return 1;
    }
    return 0;
}
