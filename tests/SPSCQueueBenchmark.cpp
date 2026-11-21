#include "SPSCQueue.hpp"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <memory>
#include <new>
#include <string>
#include <thread>
#include <vector>

#include <pthread.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{

constexpr uint32_t BenchmarkQueueSize = 1024;

struct Options
{
    uint32_t Runs = 5;
    uint64_t SameProcessMessages = 500000;
    uint64_t ProcessMessages = 100000;
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

    uint64_t Sequence;
    uint64_t TimeStamp;
    uint8_t Bytes[PayloadSize - 16];
};

// Snapshot of the original implementation. It is used only to provide a
// repeatable A/B baseline for the benchmark.
template <class T, uint32_t N>
class LegacySPSCQueue
{
public:
    static_assert(N && !(N & (N - 1)), "N must be a power of 2");

    LegacySPSCQueue()
    {
        Reset();
    }

    void Reset()
    {
        m_Head.store(0, std::memory_order_relaxed);
        m_Tail.store(0, std::memory_order_relaxed);
        std::memset(m_Data, 0, sizeof(T) * N);
    }

    bool Push(const T& value)
    {
        const int writeIndex = m_Tail.load(std::memory_order_relaxed);
        const int nextWriteIndex = (writeIndex + 1) & (N - 1);
        if(nextWriteIndex == m_Head.load(std::memory_order_acquire))
        {
            return false;
        }
        std::memcpy(&m_Data[writeIndex], &value, sizeof(T));
        m_Tail.store(nextWriteIndex, std::memory_order_release);
        return true;
    }

    bool Pop(T& value)
    {
        const int readIndex = m_Head.load(std::memory_order_relaxed);
        if(readIndex == m_Tail.load(std::memory_order_acquire))
        {
            return false;
        }
        std::memcpy(&value, &m_Data[readIndex], sizeof(T));
        m_Head.store((readIndex + 1) & (N - 1), std::memory_order_release);
        return true;
    }

private:
    alignas(128) T m_Data[N] = {};
    alignas(128) std::atomic<int> m_Head;
    alignas(128) std::atomic<int> m_Tail;
};

struct CpuPair
{
    int Producer = -1;
    int Consumer = -1;
};

struct RunSync
{
    std::atomic<int> Ready{0};
    std::atomic<bool> Start{false};
    std::atomic<bool> Produced{false};
    std::atomic<bool> ProducerValid{true};
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

int ReadTopologyValue(int cpu, const char* name)
{
    std::ifstream input(std::string("/sys/devices/system/cpu/cpu") +
                        std::to_string(cpu) + "/topology/" + name);
    int value = -1;
    input >> value;
    return value;
}

bool IsDifferentCore(int lhs, int rhs)
{
    const int lhsPackage = ReadTopologyValue(lhs, "physical_package_id");
    const int rhsPackage = ReadTopologyValue(rhs, "physical_package_id");
    const int lhsCore = ReadTopologyValue(lhs, "core_id");
    const int rhsCore = ReadTopologyValue(rhs, "core_id");
    return lhsPackage != rhsPackage || lhsCore != rhsCore;
}

CpuPair SelectCpuPair()
{
    const std::vector<int> cpus = GetAffinityCpus();
    CpuPair pair;
    if(cpus.empty())
    {
        return pair;
    }

    pair.Producer = cpus.front();
    pair.Consumer = cpus.front();
    const int producerPackage =
        ReadTopologyValue(pair.Producer, "physical_package_id");
    for(const int cpu : cpus)
    {
        if(ReadTopologyValue(cpu, "physical_package_id") == producerPackage &&
           IsDifferentCore(pair.Producer, cpu))
        {
            pair.Consumer = cpu;
            return pair;
        }
    }
    for(const int cpu : cpus)
    {
        if(IsDifferentCore(pair.Producer, cpu))
        {
            pair.Consumer = cpu;
            break;
        }
    }
    return pair;
}

void PinCurrentThread(int cpu)
{
    if(cpu < 0)
    {
        return;
    }
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpu, &cpuSet);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet);
}

template <size_t PayloadSize>
Payload<PayloadSize> MakePayload(uint64_t sequence)
{
    Payload<PayloadSize> payload{};
    payload.Sequence = sequence;
    payload.TimeStamp = NowNs();
    for(size_t i = 0; i < sizeof(payload.Bytes); ++i)
    {
        payload.Bytes[i] = static_cast<uint8_t>(sequence + i);
    }
    return payload;
}

template <size_t PayloadSize>
bool IsValidPayload(const Payload<PayloadSize>& payload, uint64_t expected)
{
    if(payload.Sequence != expected)
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

RunResult Summarize(std::vector<uint64_t>& latency, uint64_t begin, uint64_t end,
                    bool valid)
{
    RunResult result;
    result.Valid = valid;
    if(latency.empty())
    {
        return result;
    }

    std::sort(latency.begin(), latency.end());
    result.P50Ns = latency[latency.size() * 50 / 100];
    result.P99Ns = latency[latency.size() * 99 / 100];
    const double seconds = static_cast<double>(end - begin) / 1e9;
    result.ThroughputMpps =
        static_cast<double>(latency.size()) / seconds / 1000000.0;
    return result;
}

template <class Queue, size_t PayloadSize>
RunResult RunSameProcess(uint64_t messageCount, const CpuPair& cpus)
{
    using Message = Payload<PayloadSize>;
    Queue queue;
    std::atomic<int> ready{0};
    std::atomic<bool> start{false};
    std::vector<uint64_t> latency;
    latency.reserve(static_cast<size_t>(messageCount));
    std::atomic<bool> valid{true};

    std::thread producer([&]() {
        PinCurrentThread(cpus.Producer);
        ready.fetch_add(1, std::memory_order_release);
        while(!start.load(std::memory_order_acquire))
        {
            CpuRelax();
        }
        for(uint64_t sequence = 0; sequence < messageCount; ++sequence)
        {
            const Message message = MakePayload<PayloadSize>(sequence);
            while(!queue.Push(message))
            {
                CpuRelax();
            }
        }
    });

    std::thread consumer([&]() {
        PinCurrentThread(cpus.Consumer);
        ready.fetch_add(1, std::memory_order_release);
        while(!start.load(std::memory_order_acquire))
        {
            CpuRelax();
        }

        Message message{};
        uint64_t expected = 0;
        while(expected < messageCount)
        {
            if(queue.Pop(message))
            {
                if(!IsValidPayload<PayloadSize>(message, expected))
                {
                    valid.store(false, std::memory_order_relaxed);
                    return;
                }
                latency.push_back(NowNs() - message.TimeStamp);
                ++expected;
            }
            else
            {
                CpuRelax();
            }
        }
    });

    while(ready.load(std::memory_order_acquire) != 2)
    {
        CpuRelax();
    }
    const uint64_t begin = NowNs();
    start.store(true, std::memory_order_release);
    producer.join();
    consumer.join();
    const uint64_t end = NowNs();
    return Summarize(latency, begin, end,
                     valid.load(std::memory_order_relaxed));
}

template <class Queue, size_t PayloadSize>
RunResult RunCrossProcess(uint64_t messageCount, const CpuPair& cpus)
{
    using Message = Payload<PayloadSize>;
    void* queueMemory =
        mmap(nullptr, sizeof(Queue), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    void* syncMemory =
        mmap(nullptr, sizeof(RunSync), PROT_READ | PROT_WRITE,
             MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if(queueMemory == MAP_FAILED || syncMemory == MAP_FAILED)
    {
        if(queueMemory != MAP_FAILED)
        {
            munmap(queueMemory, sizeof(Queue));
        }
        if(syncMemory != MAP_FAILED)
        {
            munmap(syncMemory, sizeof(RunSync));
        }
        return RunResult{};
    }

    auto* queue = ::new(queueMemory) Queue();
    auto* sync = ::new(syncMemory) RunSync();
    const pid_t child = fork();
    if(child == 0)
    {
        PinCurrentThread(cpus.Producer);
        sync->Ready.fetch_add(1, std::memory_order_release);
        while(!sync->Start.load(std::memory_order_acquire))
        {
            CpuRelax();
        }
        for(uint64_t sequence = 0; sequence < messageCount; ++sequence)
        {
            const Message message = MakePayload<PayloadSize>(sequence);
            while(!queue->Push(message))
            {
                CpuRelax();
            }
        }
        sync->Produced.store(true, std::memory_order_release);
        _exit(sync->ProducerValid.load(std::memory_order_relaxed) ? 0 : 2);
    }
    if(child < 0)
    {
        queue->~Queue();
        sync->~RunSync();
        munmap(queueMemory, sizeof(Queue));
        munmap(syncMemory, sizeof(RunSync));
        return RunResult{};
    }

    PinCurrentThread(cpus.Consumer);
    while(sync->Ready.load(std::memory_order_acquire) == 0)
    {
        CpuRelax();
    }

    std::vector<uint64_t> latency;
    latency.reserve(static_cast<size_t>(messageCount));
    bool valid = true;
    Message message{};
    const uint64_t begin = NowNs();
    sync->Start.store(true, std::memory_order_release);

    uint64_t expected = 0;
    while(expected < messageCount)
    {
        if(queue->Pop(message))
        {
            if(!IsValidPayload<PayloadSize>(message, expected))
            {
                valid = false;
                break;
            }
            latency.push_back(NowNs() - message.TimeStamp);
            ++expected;
        }
        else
        {
            CpuRelax();
        }
    }
    const uint64_t end = NowNs();

    int status = 0;
    waitpid(child, &status, 0);
    if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
    {
        valid = false;
    }

    queue->~Queue();
    sync->~RunSync();
    munmap(queueMemory, sizeof(Queue));
    munmap(syncMemory, sizeof(RunSync));
    return Summarize(latency, begin, end, valid);
}

double Median(std::vector<double> values)
{
    if(values.empty())
    {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    if(values.size() % 2 == 0)
    {
        return (values[middle - 1] + values[middle]) / 2.0;
    }
    return values[middle];
}

uint64_t Median(std::vector<uint64_t> values)
{
    if(values.empty())
    {
        return 0;
    }
    std::sort(values.begin(), values.end());
    const size_t middle = values.size() / 2;
    if(values.size() % 2 == 0)
    {
        return (values[middle - 1] + values[middle]) / 2;
    }
    return values[middle];
}

bool AddResult(const RunResult& result, std::vector<double>& throughput,
               std::vector<uint64_t>& p50, std::vector<uint64_t>& p99)
{
    throughput.push_back(result.ThroughputMpps);
    p50.push_back(result.P50Ns);
    p99.push_back(result.P99Ns);
    return result.Valid;
}

void PrintSummary(const char* name, const std::vector<double>& baselineThroughput,
                  const std::vector<uint64_t>& baselineP50,
                  const std::vector<uint64_t>& baselineP99,
                  const std::vector<double>& candidateThroughput,
                  const std::vector<uint64_t>& candidateP50,
                  const std::vector<uint64_t>& candidateP99)
{
    const double baselineMpps = Median(baselineThroughput);
    const double candidateMpps = Median(candidateThroughput);
    const double improvement =
        baselineMpps == 0 ? 0 : (candidateMpps / baselineMpps - 1.0) * 100.0;
    std::printf(
        "%-15s baseline %.2f Mmsg/s p50=%lu p99=%lu | "
        "candidate %.2f Mmsg/s p50=%lu p99=%lu | throughput %+.1f%%\n",
        name, baselineMpps,
        static_cast<unsigned long>(Median(baselineP50)),
        static_cast<unsigned long>(Median(baselineP99)),
        candidateMpps,
        static_cast<unsigned long>(Median(candidateP50)),
        static_cast<unsigned long>(Median(candidateP99)), improvement);
}

template <size_t PayloadSize>
bool RunPayloadSuite(const Options& options, const CpuPair& cpus)
{
    using Message = Payload<PayloadSize>;
    using Candidate = SHMIPC::SPSCQueue<Message, BenchmarkQueueSize>;
    using Baseline = LegacySPSCQueue<Message, BenchmarkQueueSize>;

    const uint64_t warmupCount = std::min<uint64_t>(20000,
                                                    options.SameProcessMessages);
    RunSameProcess<Baseline, PayloadSize>(warmupCount, cpus);
    RunSameProcess<Candidate, PayloadSize>(warmupCount, cpus);

    std::vector<double> baselineSameThroughput;
    std::vector<uint64_t> baselineSameP50;
    std::vector<uint64_t> baselineSameP99;
    std::vector<double> candidateSameThroughput;
    std::vector<uint64_t> candidateSameP50;
    std::vector<uint64_t> candidateSameP99;
    std::vector<double> baselineProcessThroughput;
    std::vector<uint64_t> baselineProcessP50;
    std::vector<uint64_t> baselineProcessP99;
    std::vector<double> candidateProcessThroughput;
    std::vector<uint64_t> candidateProcessP50;
    std::vector<uint64_t> candidateProcessP99;

    bool allValid = true;
    for(uint32_t run = 0; run < options.Runs; ++run)
    {
        const bool baselineFirst = run % 2 == 0;
        RunResult baselineSame;
        RunResult candidateSame;
        if(baselineFirst)
        {
            baselineSame =
                RunSameProcess<Baseline, PayloadSize>(
                    options.SameProcessMessages, cpus);
            candidateSame =
                RunSameProcess<Candidate, PayloadSize>(
                    options.SameProcessMessages, cpus);
        }
        else
        {
            candidateSame =
                RunSameProcess<Candidate, PayloadSize>(
                    options.SameProcessMessages, cpus);
            baselineSame =
                RunSameProcess<Baseline, PayloadSize>(
                    options.SameProcessMessages, cpus);
        }
        allValid = AddResult(baselineSame, baselineSameThroughput,
                             baselineSameP50, baselineSameP99) &&
                   allValid;
        allValid = AddResult(candidateSame, candidateSameThroughput,
                             candidateSameP50, candidateSameP99) &&
                   allValid;

        RunResult baselineProcess;
        RunResult candidateProcess;
        if(baselineFirst)
        {
            baselineProcess =
                RunCrossProcess<Baseline, PayloadSize>(
                    options.ProcessMessages, cpus);
            candidateProcess =
                RunCrossProcess<Candidate, PayloadSize>(
                    options.ProcessMessages, cpus);
        }
        else
        {
            candidateProcess =
                RunCrossProcess<Candidate, PayloadSize>(
                    options.ProcessMessages, cpus);
            baselineProcess =
                RunCrossProcess<Baseline, PayloadSize>(
                    options.ProcessMessages, cpus);
        }
        allValid = AddResult(baselineProcess, baselineProcessThroughput,
                             baselineProcessP50, baselineProcessP99) &&
                   allValid;
        allValid = AddResult(candidateProcess, candidateProcessThroughput,
                             candidateProcessP50, candidateProcessP99) &&
                   allValid;
    }

    const std::string prefix = std::to_string(PayloadSize) + "B same";
    const std::string processPrefix = std::to_string(PayloadSize) + "B cross";
    PrintSummary(prefix.c_str(), baselineSameThroughput, baselineSameP50,
                 baselineSameP99, candidateSameThroughput, candidateSameP50,
                 candidateSameP99);
    PrintSummary(processPrefix.c_str(), baselineProcessThroughput,
                 baselineProcessP50, baselineProcessP99,
                 candidateProcessThroughput, candidateProcessP50,
                 candidateProcessP99);
    return allValid;
}

void DoNotOptimize(const void* value)
{
    asm volatile("" : : "r"(value) : "memory");
}

template <class Queue>
double MeasureReset(const char* name, bool baseline)
{
    auto queue = std::make_unique<Queue>();
    queue->Reset();
    DoNotOptimize(queue.get());

    const uint32_t iterations = baseline ? 20 : 5000;
    const uint64_t begin = NowNs();
    for(uint32_t i = 0; i < iterations; ++i)
    {
        queue->Reset();
        DoNotOptimize(queue.get());
    }
    const uint64_t end = NowNs();
    const double averageNs =
        static_cast<double>(end - begin) / iterations;
    std::printf("Reset %-20s %.1f ns/op (size=%.2f MiB)\n", name, averageNs,
                static_cast<double>(sizeof(Queue)) / 1024.0 / 1024.0);
    return averageNs;
}

bool ParseOptions(int argc, char* argv[], Options& options)
{
    for(int i = 1; i < argc; ++i)
    {
        if(std::strcmp(argv[i], "--runs") == 0 && i + 1 < argc)
        {
            options.Runs = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
        }
        else if(std::strcmp(argv[i], "--same-count") == 0 && i + 1 < argc)
        {
            options.SameProcessMessages =
                std::strtoull(argv[++i], nullptr, 10);
        }
        else if(std::strcmp(argv[i], "--process-count") == 0 && i + 1 < argc)
        {
            options.ProcessMessages =
                std::strtoull(argv[++i], nullptr, 10);
        }
        else
        {
            std::fprintf(stderr,
                         "Usage: %s [--runs N] [--same-count N] "
                         "[--process-count N]\n",
                         argv[0]);
            return false;
        }
    }
    return options.Runs > 0 && options.SameProcessMessages > 0 &&
           options.ProcessMessages > 0;
}

} // namespace

int main(int argc, char* argv[])
{
    Options options;
    if(!ParseOptions(argc, argv, options))
    {
        return 1;
    }

    const CpuPair cpus = SelectCpuPair();
    std::printf("SPSC A/B benchmark: producer CPU %d consumer CPU %d, "
                "%u rounds\n",
                cpus.Producer, cpus.Consumer, options.Runs);

    bool valid = RunPayloadSuite<64>(options, cpus) &&
                 RunPayloadSuite<1024>(options, cpus);

    using Payload1K = Payload<1024>;
    using SmallQueue = SHMIPC::SPSCQueue<Payload1K, 64>;
    using LargeQueue = SHMIPC::SPSCQueue<Payload1K, 8192>;
    using LegacySmallQueue = LegacySPSCQueue<Payload1K, 64>;
    using LegacyLargeQueue = LegacySPSCQueue<Payload1K, 8192>;

    const double legacySmall =
        MeasureReset<LegacySmallQueue>("baseline 64 KiB", true);
    const double legacyLarge =
        MeasureReset<LegacyLargeQueue>("baseline 8 MiB", true);
    const double candidateSmall =
        MeasureReset<SmallQueue>("candidate 64 KiB", false);
    const double candidateLarge =
        MeasureReset<LargeQueue>("candidate 8 MiB", false);
    std::printf("Reset scaling baseline x%.1f, candidate x%.1f, "
                "8 MiB speedup x%.1f\n",
                legacySmall == 0 ? 0 : legacyLarge / legacySmall,
                candidateSmall == 0 ? 0 : candidateLarge / candidateSmall,
                candidateLarge == 0 ? 0 : legacyLarge / candidateLarge);

    if(!valid)
    {
        std::fprintf(stderr, "SPSC benchmark validation failed\n");
        return 1;
    }
    return 0;
}
