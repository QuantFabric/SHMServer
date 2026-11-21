#include "SHMServer.hpp"
#include "SHMConnection.hpp"

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
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <sys/mman.h>
#include <sys/wait.h>
#include <unistd.h>

namespace
{

template <size_t MessageSize>
struct BenchmarkMessage
{
    static_assert(MessageSize >= 20 && MessageSize % 8 == 0,
                  "message size must include metadata");

    uint64_t MsgID;
    uint64_t TimeStamp;
    int32_t ChannelID;
    uint8_t Data[MessageSize - 20];
};

struct ServerConf : SHMIPC::CommonConf
{
    static const uint32_t ChannelSize = 16;
    static const bool Publish = false;
    static const bool Performance = true;
};

struct PublishServerConf : SHMIPC::CommonConf
{
    static const bool Publish = true;
    static const bool Performance = true;
};

struct ClientConf : SHMIPC::CommonConf
{
    static const uint32_t ChannelSize = 16;
    static const bool Publish = false;
    static const bool Performance = true;
};

struct PublishClientConf : SHMIPC::CommonConf
{
    static const bool Publish = true;
    static const bool Performance = true;
};

uint64_t NowNs()
{
    return static_cast<uint64_t>(
        std::chrono::steady_clock::now().time_since_epoch().count());
}

void PinCurrentThread(int cpu)
{
    cpu_set_t cpuSet;
    CPU_ZERO(&cpuSet);
    CPU_SET(cpu, &cpuSet);
    pthread_setaffinity_np(pthread_self(), sizeof(cpuSet), &cpuSet);
}

template <size_t MessageSize>
class EchoServer
    : public SHMIPC::SHMServer<BenchmarkMessage<MessageSize>, ServerConf>
{
public:
    void HandleMsg() override
    {
#ifdef SHM_HEAD_BASELINE
        SHMIPC::TChannelMsg<BenchmarkMessage<MessageSize>> message;
#else
        SHMIPC::TDataMsg<BenchmarkMessage<MessageSize>> message;
#endif
        while(this->m_pRecvQueue->Pop(message))
        {
            if(!this->m_pSendQueue->Push(message))
            {
                return;
            }
        }
    }
};

template <size_t MessageSize>
class Publisher
    : public SHMIPC::SHMServer<BenchmarkMessage<MessageSize>,
                               PublishServerConf>
{
public:
    void HandleMsg() override
    {
    }
};

struct LatencySummary
{
    uint64_t P50Ns = 0;
    uint64_t P99Ns = 0;
};

LatencySummary Summarize(std::vector<uint64_t> latency)
{
    LatencySummary result;
    if(latency.empty())
    {
        return result;
    }
    std::sort(latency.begin(), latency.end());
    result.P50Ns = latency[latency.size() * 50 / 100];
    result.P99Ns = latency[latency.size() * 99 / 100];
    return result;
}

template <size_t MessageSize>
bool RunEcho(size_t clientCount, uint64_t messagesPerClient)
{
    using Message = BenchmarkMessage<MessageSize>;
    const std::string serverName =
        "SHMPerfEcho_" + std::to_string(MessageSize) + "_" +
        std::to_string(getpid()) + "_" + std::to_string(clientCount);
    const std::string shmName = "/" + serverName + ".shm";
    bool valid = true;
    std::vector<uint64_t> allLatency;
    allLatency.reserve(clientCount * messagesPerClient);
    uint64_t beginNs = 0;
    uint64_t endNs = 0;

    {
        EchoServer<MessageSize> server;
        server.Start(serverName, 0);
        std::vector<std::unique_ptr<
            SHMIPC::SHMConnection<Message, ClientConf>>>
            clients;

        for(size_t clientIndex = 0; clientIndex < clientCount; ++clientIndex)
        {
            auto client = std::make_unique<
                SHMIPC::SHMConnection<Message, ClientConf>>(
                "EchoClient" + std::to_string(clientIndex));
            client->Start(serverName);
            clients.push_back(std::move(client));
        }

        PinCurrentThread(8);
        beginNs = NowNs();
        for(size_t clientIndex = 0; clientIndex < clientCount; ++clientIndex)
        {
            for(uint64_t i = 0; i < messagesPerClient; ++i)
            {
                Message sending{};
                sending.MsgID = i;
                const uint64_t startNs = NowNs();
                while(!clients[clientIndex]->Push(sending))
                {
                    __builtin_ia32_pause();
                }

                Message received{};
                while(!clients[clientIndex]->Pop(received))
                {
                    __builtin_ia32_pause();
                }
                if(received.MsgID != i)
                {
                    valid = false;
                    break;
                }
                allLatency.push_back(NowNs() - startNs);
            }
        }
        endNs = NowNs();

        for(auto& client : clients)
        {
            client->Stop();
            client->Join();
        }
        server.Stop();
        server.Join();
    }

    shm_unlink(shmName.c_str());
    const LatencySummary latency = Summarize(allLatency);
    const double seconds = clientCount == 0
                               ? 0
                               : static_cast<double>(endNs - beginNs) / 1e9;
    const double millisecondsPerSecond =
        seconds == 0 ? 0 : allLatency.size() / seconds / 1000000.0;
    std::fprintf(stderr,
                 "RESULT echo_%zuB_%zuC p50=%lu p99=%lu throughput=%.2f "
                 "valid=%d\n",
                 MessageSize, clientCount,
                 static_cast<unsigned long>(latency.P50Ns),
                 static_cast<unsigned long>(latency.P99Ns),
                 millisecondsPerSecond, valid ? 1 : 0);
    return valid;
}

template <size_t MessageSize>
bool RunPublish(size_t clientCount, uint64_t messages)
{
    using Message = BenchmarkMessage<MessageSize>;
    const std::string serverName =
        "SHMPerfPub_" + std::to_string(MessageSize) + "_" +
        std::to_string(getpid()) + "_" + std::to_string(clientCount);
    const std::string shmName = "/" + serverName + ".shm";
    bool valid = true;
    uint64_t beginNs = 0;
    uint64_t endNs = 0;
    std::vector<std::vector<uint64_t>> latencies(clientCount);
    std::vector<uint64_t> sendTimes(messages);

    {
        Publisher<MessageSize> server;
        server.Start(serverName, 0);
        std::vector<std::unique_ptr<
            SHMIPC::SHMConnection<Message, PublishClientConf>>>
            clients;
        std::vector<std::thread> consumers;

        for(size_t clientIndex = 0; clientIndex < clientCount; ++clientIndex)
        {
            auto client = std::make_unique<
                SHMIPC::SHMConnection<Message, PublishClientConf>>(
                "PubClient" + std::to_string(clientIndex));
            client->Start(serverName);
            clients.push_back(std::move(client));
        }

        std::atomic<bool> start{false};
        for(size_t clientIndex = 0; clientIndex < clientCount; ++clientIndex)
        {
            consumers.emplace_back([&, clientIndex]() {
                PinCurrentThread(8 + static_cast<int>(clientIndex) * 2);
                while(!start.load(std::memory_order_acquire))
                {
                    __builtin_ia32_pause();
                }
                auto& latency = latencies[clientIndex];
                latency.reserve(messages);
                Message received{};
                uint64_t expected = 0;
                while(expected < messages)
                {
                    if(clients[clientIndex]->Pop(received))
                    {
                        if(received.MsgID != expected)
                        {
                            valid = false;
                        }
                        latency.push_back(
                            NowNs() - sendTimes[received.MsgID]);
                        ++expected;
                    }
                    else
                    {
                        __builtin_ia32_pause();
                    }
                }
            });
        }

        PinCurrentThread(20);
        beginNs = NowNs();
        start.store(true, std::memory_order_release);
        for(uint64_t i = 0; i < messages; ++i)
        {
            Message sending{};
            sending.MsgID = i;
            sending.TimeStamp = NowNs();
            sendTimes[i] = sending.TimeStamp;
            while(!server.Push(sending))
            {
                __builtin_ia32_pause();
            }
        }

        for(auto& consumer : consumers)
        {
            consumer.join();
        }
        endNs = NowNs();
        for(auto& client : clients)
        {
            client->Stop();
            client->Join();
        }
        server.Stop();
        server.Join();
    }

    shm_unlink(shmName.c_str());
    std::vector<uint64_t> allLatency;
    for(const auto& latency : latencies)
    {
        allLatency.insert(allLatency.end(), latency.begin(), latency.end());
    }
    const LatencySummary latency = Summarize(allLatency);
    const double seconds = static_cast<double>(endNs - beginNs) / 1e9;
    const double deliveriesPerSecond =
        seconds == 0 ? 0 : allLatency.size() / seconds / 1000000.0;
    std::fprintf(stderr,
                 "RESULT publish_%zuB_%zuC p50=%lu p99=%lu "
                 "throughput=%.2f valid=%d\n",
                 MessageSize, clientCount,
                 static_cast<unsigned long>(latency.P50Ns),
                 static_cast<unsigned long>(latency.P99Ns),
                 deliveriesPerSecond, valid ? 1 : 0);
    return valid;
}

struct ProcessSync
{
    std::atomic<int> Ready{0};
    std::atomic<bool> Start{false};
    std::atomic<uint64_t> SendTimes[1024];
};

struct ProcessResult
{
    uint64_t Count = 0;
    uint64_t P50Ns = 0;
    uint64_t P99Ns = 0;
    uint32_t Valid = 0;
};

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

ProcessSync* MapProcessSync(const char* name, bool create)
{
    const int flags = create ? O_CREAT | O_RDWR | O_EXCL : O_RDWR;
    const int fd = shm_open(name, flags, 0600);
    if(fd == -1)
    {
        return nullptr;
    }
    if(create && ftruncate(fd, sizeof(ProcessSync)))
    {
        close(fd);
        return nullptr;
    }
    auto* sync = static_cast<ProcessSync*>(
        mmap(0, sizeof(ProcessSync), PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0));
    close(fd);
    if(sync == MAP_FAILED)
    {
        return nullptr;
    }
    if(create)
    {
        new(sync) ProcessSync();
    }
    return sync;
}

template <size_t MessageSize>
bool RunPublishServerMode(const std::string& serverName, int readyFd,
                          int stopFd, int startFd, uint64_t messages,
                          const std::string& syncName, int producerCpu)
{
    auto* sync = MapProcessSync(syncName.c_str(), false);
    if(!sync)
    {
        return false;
    }
    Publisher<MessageSize> server;
    server.Start(serverName, 0);
    const char ready = 1;
    if(write(readyFd, &ready, sizeof(ready)) != sizeof(ready))
    {
        return false;
    }
    char stop = 0;
    if(read(startFd, &stop, sizeof(stop)) != sizeof(stop))
    {
        server.Stop();
        server.Join();
        return false;
    }

    PinCurrentThread(producerCpu);
    for(uint64_t i = 0; i < messages; ++i)
    {
        BenchmarkMessage<MessageSize> sending{};
        sending.MsgID = i;
        sending.TimeStamp = NowNs();
        sync->SendTimes[i].store(sending.TimeStamp,
                                 std::memory_order_relaxed);
        while(!server.Push(sending))
        {
            __builtin_ia32_pause();
        }
    }

    const ssize_t bytes = read(stopFd, &stop, sizeof(stop));
    server.Stop();
    server.Join();
    munmap(sync, sizeof(ProcessSync));
    return bytes == sizeof(stop);
}

template <size_t MessageSize>
bool RunPublishClientMode(const std::string& clientName,
                          const std::string& serverName,
                          uint64_t messages, const std::string& syncName,
                          const std::string& resultPath, int applicationCpu)
{
    auto* sync = MapProcessSync(syncName.c_str(), false);
    if(!sync)
    {
        return false;
    }

    using Message = BenchmarkMessage<MessageSize>;
    SHMIPC::SHMConnection<Message, PublishClientConf> client(clientName);
    client.Start(serverName);
    PinCurrentThread(applicationCpu);
    sync->Ready.fetch_add(1, std::memory_order_release);
    while(!sync->Start.load(std::memory_order_acquire))
    {
        __builtin_ia32_pause();
    }

    std::vector<uint64_t> latency;
    latency.reserve(messages);
    Message received{};
    uint64_t expected = 0;
    bool valid = true;
    while(expected < messages)
    {
        if(client.Pop(received))
        {
            if(received.MsgID != expected)
            {
                valid = false;
            }
            const uint64_t sendTime =
                received.MsgID < messages
                    ? sync->SendTimes[received.MsgID].load(
                          std::memory_order_relaxed)
                    : NowNs();
            latency.push_back(NowNs() - sendTime);
            ++expected;
        }
        else
        {
            __builtin_ia32_pause();
        }
    }

    const LatencySummary summary = Summarize(latency);
    ProcessResult result{};
    result.Count = latency.size();
    result.P50Ns = summary.P50Ns;
    result.P99Ns = summary.P99Ns;
    result.Valid = valid ? 1 : 0;
    FILE* output = std::fopen(resultPath.c_str(), "wb");
    if(!output ||
       std::fwrite(&result, sizeof(result), 1, output) != 1)
    {
        if(output)
        {
            std::fclose(output);
        }
        valid = false;
    }
    else
    {
        std::fclose(output);
    }

    client.Stop();
    client.Join();
    munmap(sync, sizeof(ProcessSync));
    return valid;
}

template <size_t MessageSize>
bool RunPublishProcesses(size_t clientCount, uint64_t messages)
{
    const pid_t processId = getpid();
    const std::string serverName =
        "SHMPerfPubProc_" + std::to_string(MessageSize) + "_" +
        std::to_string(processId) + "_" + std::to_string(clientCount);
    const std::string shmName = "/" + serverName + ".shm";
    const std::string syncName =
        "/SHMPubSync_" + std::to_string(processId) + "_" +
        std::to_string(MessageSize) + "_" + std::to_string(clientCount);
    const std::string executable = "/proc/self/exe";
    const std::vector<int> cpus = GetAffinityCpus();

    auto* sync = MapProcessSync(syncName.c_str(), true);
    if(!sync)
    {
        return false;
    }

    int serverReady[2];
    int serverStop[2];
    int serverStart[2];
    if(pipe(serverReady) || pipe(serverStop) || pipe(serverStart))
    {
        shm_unlink(syncName.c_str());
        return false;
    }

    const pid_t serverPid = fork();
    if(serverPid == 0)
    {
        close(serverReady[0]);
        close(serverStop[1]);
        close(serverStart[1]);
        const std::string size = std::to_string(MessageSize);
        const std::string readyFd = std::to_string(serverReady[1]);
        const std::string stopFd = std::to_string(serverStop[0]);
        const std::string startFd = std::to_string(serverStart[0]);
        const std::string messageCount = std::to_string(messages);
        const int producerCpu =
            cpus.empty() ? -1 : cpus.back();
        const std::string producer = std::to_string(producerCpu);
        execl(executable.c_str(), executable.c_str(), "--publish-server",
              size.c_str(), serverName.c_str(), readyFd.c_str(),
              stopFd.c_str(), startFd.c_str(), messageCount.c_str(),
              syncName.c_str(), producer.c_str(),
              static_cast<char*>(nullptr));
        _exit(127);
    }
    close(serverReady[1]);
    close(serverStop[0]);
    close(serverStart[0]);
    fcntl(serverReady[0], F_SETFD, FD_CLOEXEC);
    fcntl(serverStop[1], F_SETFD, FD_CLOEXEC);
    fcntl(serverStart[1], F_SETFD, FD_CLOEXEC);

    struct pollfd readyPoll
    {
        serverReady[0], POLLIN, 0
    };
    if(serverPid < 0 || poll(&readyPoll, 1, 10000) <= 0)
    {
        shm_unlink(syncName.c_str());
        return false;
    }
    char readyByte = 0;
    if(read(serverReady[0], &readyByte, sizeof(readyByte)) !=
       sizeof(readyByte))
    {
        shm_unlink(syncName.c_str());
        return false;
    }

    std::vector<pid_t> clients;
    std::vector<std::string> resultPaths;
    clients.reserve(clientCount);
    resultPaths.reserve(clientCount);
    for(size_t clientIndex = 0; clientIndex < clientCount; ++clientIndex)
    {
        const std::string clientName =
            "PubProcClient" + std::to_string(clientIndex);
        const std::string resultPath =
            "/tmp/SHMPubResult_" + std::to_string(processId) + "_" +
            std::to_string(MessageSize) + "_" +
            std::to_string(clientCount) + "_" +
            std::to_string(clientIndex);
        resultPaths.push_back(resultPath);

        const int applicationCpu =
            cpus.empty()
                ? -1
                : cpus[(clientIndex + clientCount / 2 + 1) % cpus.size()];
        const pid_t clientPid = fork();
        if(clientPid == 0)
        {
            const std::string size = std::to_string(MessageSize);
            const std::string messageCount = std::to_string(messages);
            const std::string application =
                std::to_string(applicationCpu);
            execl(executable.c_str(), executable.c_str(),
                  "--publish-client", size.c_str(), clientName.c_str(),
                  serverName.c_str(), messageCount.c_str(), syncName.c_str(),
                  resultPath.c_str(), application.c_str(),
                  static_cast<char*>(nullptr));
            _exit(127);
        }
        clients.push_back(clientPid);
    }

    const uint64_t readyDeadline = NowNs() + 30000000000ULL;
    while(sync->Ready.load(std::memory_order_acquire) <
              static_cast<int>(clientCount) &&
          NowNs() < readyDeadline)
    {
        usleep(1000);
    }
    const bool allReady =
        sync->Ready.load(std::memory_order_acquire) ==
        static_cast<int>(clientCount);

    const uint64_t beginNs = NowNs();
    sync->Start.store(true, std::memory_order_release);
    write(serverStart[1], &readyByte, sizeof(readyByte));

    bool valid = allReady;
    for(const pid_t pid : clients)
    {
        int status = 0;
        waitpid(pid, &status, 0);
        if(!WIFEXITED(status) || WEXITSTATUS(status) != 0)
        {
            valid = false;
        }
    }
    const uint64_t endNs = NowNs();

    const char stop = 1;
    write(serverStop[1], &stop, sizeof(stop));
    int serverStatus = 0;
    waitpid(serverPid, &serverStatus, 0);
    if(!WIFEXITED(serverStatus) || WEXITSTATUS(serverStatus) != 0)
    {
        valid = false;
    }

    std::vector<ProcessResult> results;
    results.reserve(clientCount);
    for(const std::string& path : resultPaths)
    {
        ProcessResult result{};
        FILE* input = std::fopen(path.c_str(), "rb");
        if(input &&
           std::fread(&result, sizeof(result), 1, input) == 1)
        {
            results.push_back(result);
        }
        else
        {
            valid = false;
        }
        if(input)
        {
            std::fclose(input);
        }
        unlink(path.c_str());
    }

    close(serverReady[0]);
    close(serverStop[1]);
    close(serverStart[1]);
    munmap(sync, sizeof(ProcessSync));
    shm_unlink(syncName.c_str());
    shm_unlink(shmName.c_str());

    std::vector<uint64_t> p50Values;
    uint64_t maxP99 = 0;
    uint64_t deliveries = 0;
    for(const ProcessResult& result : results)
    {
        p50Values.push_back(result.P50Ns);
        maxP99 = std::max(maxP99, result.P99Ns);
        deliveries += result.Count;
        valid = valid && result.Valid;
    }
    std::sort(p50Values.begin(), p50Values.end());
    const uint64_t p50 =
        p50Values.empty() ? 0 : p50Values[p50Values.size() / 2];
    const double seconds = static_cast<double>(endNs - beginNs) / 1e9;
    const double deliveriesPerSecond =
        seconds == 0 ? 0 : deliveries / seconds / 1000000.0;
    std::fprintf(stderr,
                 "RESULT publish_proc_%zuB_%zuC p50=%lu p99=%lu "
                 "throughput=%.2f valid=%d\n",
                 MessageSize, clientCount,
                 static_cast<unsigned long>(p50),
                 static_cast<unsigned long>(maxP99),
                 deliveriesPerSecond, valid ? 1 : 0);
    return valid;
}

bool RunPublishServerBySize(size_t size, const std::string& serverName,
                            int readyFd, int stopFd, int startFd,
                            uint64_t messages, const std::string& syncName,
                            int producerCpu)
{
    switch(size)
    {
        case 128:
            return RunPublishServerMode<128>(
                serverName, readyFd, stopFd, startFd, messages, syncName,
                producerCpu);
        case 256:
            return RunPublishServerMode<256>(
                serverName, readyFd, stopFd, startFd, messages, syncName,
                producerCpu);
        case 512:
            return RunPublishServerMode<512>(
                serverName, readyFd, stopFd, startFd, messages, syncName,
                producerCpu);
        case 1024:
            return RunPublishServerMode<1024>(
                serverName, readyFd, stopFd, startFd, messages, syncName,
                producerCpu);
        default: return false;
    }
}

bool RunPublishClientBySize(size_t size, const std::string& clientName,
                            const std::string& serverName, uint64_t messages,
                            const std::string& syncName,
                            const std::string& resultPath, int applicationCpu)
{
    switch(size)
    {
        case 128:
            return RunPublishClientMode<128>(
                clientName, serverName, messages, syncName, resultPath,
                applicationCpu);
        case 256:
            return RunPublishClientMode<256>(
                clientName, serverName, messages, syncName, resultPath,
                applicationCpu);
        case 512:
            return RunPublishClientMode<512>(
                clientName, serverName, messages, syncName, resultPath,
                applicationCpu);
        case 1024:
            return RunPublishClientMode<1024>(
                clientName, serverName, messages, syncName, resultPath,
                applicationCpu);
        default:
            return false;
    }
}

} // namespace

int main(int argc, char* argv[])
{
    if(argc > 1 && std::strcmp(argv[1], "--publish-server") == 0 &&
       argc == 10)
    {
        return RunPublishServerBySize(
                   std::strtoul(argv[2], nullptr, 10), argv[3],
                   std::atoi(argv[4]), std::atoi(argv[5]),
                   std::atoi(argv[6]),
                   std::strtoull(argv[7], nullptr, 10), argv[8],
                   std::atoi(argv[9]))
                   ? 0
                   : 1;
    }
    if(argc > 1 && std::strcmp(argv[1], "--publish-client") == 0 &&
       argc == 9)
    {
        return RunPublishClientBySize(
                   std::strtoul(argv[2], nullptr, 10), argv[3], argv[4],
                   std::strtoull(argv[5], nullptr, 10), argv[6], argv[7],
                   std::atoi(argv[8]))
                   ? 0
                   : 1;
    }

    if(argc > 1 && std::strcmp(argv[1], "--generation-key") == 0)
    {
        const bool valid =
            RunEcho<128>(5, 1000) &&
            RunEcho<128>(10, 500) &&
            RunEcho<1024>(5, 1000) &&
            RunEcho<1024>(10, 500) &&
            RunPublishProcesses<128>(32, 500) &&
            RunPublishProcesses<128>(256, 500) &&
            RunPublishProcesses<1024>(32, 500) &&
            RunPublishProcesses<1024>(256, 500);
        std::fprintf(stderr, "SHM control generation key benchmark %s\n",
                     valid ? "passed" : "failed");
        return valid ? 0 : 1;
    }

    if(argc > 1 && std::strcmp(argv[1], "--head-compatible") == 0)
    {
        const bool valid =
            RunEcho<128>(1, 5000) &&
            RunEcho<256>(1, 4000) &&
            RunEcho<512>(1, 3500) &&
            RunEcho<1024>(1, 3000) &&
            RunPublish<128>(1, 1000) &&
            RunPublish<256>(1, 1000) &&
            RunPublish<512>(1, 1000) &&
            RunPublish<1024>(1, 1000);
        std::fprintf(stderr, "SHM HEAD-compatible benchmark %s\n",
                     valid ? "passed" : "failed");
        return valid ? 0 : 1;
    }

    const bool valid =
        RunEcho<128>(1, 5000) &&
        RunEcho<128>(2, 2500) &&
        RunEcho<1024>(1, 3000) &&
        RunEcho<1024>(2, 1500) &&
        RunEcho<128>(5, 1000) &&
        RunEcho<128>(10, 500) &&
        RunEcho<256>(5, 1000) &&
        RunEcho<256>(10, 500) &&
        RunEcho<512>(5, 1000) &&
        RunEcho<512>(10, 500) &&
        RunEcho<1024>(5, 1000) &&
        RunEcho<1024>(10, 500) &&
        RunPublish<1024>(1, 3000) &&
        RunPublish<1024>(5, 3000) &&
        RunPublishProcesses<128>(32, 500) &&
        RunPublishProcesses<128>(64, 500) &&
        RunPublishProcesses<128>(128, 500) &&
        RunPublishProcesses<128>(256, 500) &&
        RunPublishProcesses<256>(32, 500) &&
        RunPublishProcesses<256>(64, 500) &&
        RunPublishProcesses<256>(128, 500) &&
        RunPublishProcesses<256>(256, 500) &&
        RunPublishProcesses<512>(32, 500) &&
        RunPublishProcesses<512>(64, 500) &&
        RunPublishProcesses<512>(128, 500) &&
        RunPublishProcesses<512>(256, 500) &&
        RunPublishProcesses<1024>(32, 500) &&
        RunPublishProcesses<1024>(64, 500) &&
        RunPublishProcesses<1024>(128, 500) &&
        RunPublishProcesses<1024>(256, 500);
    std::fprintf(stderr, "SHM IPC benchmark %s\n",
                 valid ? "passed" : "failed");
    return valid ? 0 : 1;
}
