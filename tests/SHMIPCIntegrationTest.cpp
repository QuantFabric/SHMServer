#include "SHMServer.hpp"
#include "SHMConnection.hpp"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <memory>
#include <string>

#include <sys/mman.h>
#include <unistd.h>

namespace
{

struct PackMessage
{
    uint64_t MsgID;
    uint64_t TimeStamp;
    int32_t ChannelID;
    char Data[100];
};

struct ServerConf : SHMIPC::CommonConf
{
    static const bool Publish = false;
    static const bool Performance = true;
};

struct ClientConf : SHMIPC::CommonConf
{
    static const bool Publish = false;
    static const bool Performance = true;
};

class EchoServer : public SHMIPC::SHMServer<PackMessage, ServerConf>
{
public:
    uint64_t HeartbeatCount() const
    {
        return heartbeatCount.load(std::memory_order_acquire);
    }

    uint64_t ClientAckCount() const
    {
        return clientAckCount.load(std::memory_order_acquire);
    }

    void HandleControlMsg(
        SHMIPC::TSharedChannel<PackMessage, ServerConf>& channel,
        const SHMIPC::TControlMsg& control) override
    {
        if(control.MsgType == SHMIPC::EMsgType::EMSG_TYPE_HEARTBEAT)
        {
            heartbeatCount.fetch_add(1, std::memory_order_release);
        }
        else if(control.MsgType ==
                SHMIPC::EMsgType::EMSG_TYPE_CLIENT_ACK)
        {
            clientAckCount.fetch_add(1, std::memory_order_release);
        }
        SHMIPC::SHMServer<PackMessage, ServerConf>::HandleControlMsg(
            channel, control);
    }

    void HandleMsg() override
    {
        SHMIPC::TDataMsg<PackMessage> message;
        while(m_pRecvQueue->Pop(message))
        {
            if(!m_pSendQueue->Push(message))
            {
                return;
            }
        }
    }

private:
    std::atomic<uint64_t> heartbeatCount{0};
    std::atomic<uint64_t> clientAckCount{0};
};

class DirectClient : public SHMIPC::SHMConnection<PackMessage, ClientConf>
{
public:
    explicit DirectClient(const std::string& name)
        : SHMIPC::SHMConnection<PackMessage, ClientConf>(name)
    {
    }

    uint64_t ControlGeneration() const
    {
        return m_Channel->ControlGeneration.load(
            std::memory_order_acquire);
    }

    bool QueueControlForTest(SHMIPC::EMsgType type)
    {
        SHMIPC::TControlMsg control{};
        control.MsgType = type;
        control.ChannelID = m_ChannelID;
        control.TimeStamp = SHMIPC::RDTSC();
        return SendControlMsg(control);
    }

    bool FillControlQueueForTest()
    {
        SHMIPC::TControlMsg control{};
        control.MsgType = SHMIPC::EMsgType::EMSG_TYPE_HEARTBEAT;
        control.ChannelID = m_ChannelID;
        for(uint32_t i = 0;
            i + 1 < SHMIPC::CommonConf::ControlQueueSize; ++i)
        {
            if(!m_Channel->ControlRecvQueue.Push(control))
            {
                return false;
            }
        }
        return true;
    }
};

bool RunRoundTrips(DirectClient& client, uint64_t count)
{
    for(uint64_t i = 0; i < count; ++i)
    {
        PackMessage sending{};
        sending.MsgID = i;
        while(!client.Push(sending))
        {
            __builtin_ia32_pause();
        }

        PackMessage received{};
        while(!client.Pop(received))
        {
            __builtin_ia32_pause();
        }
        if(received.MsgID != i)
        {
            std::fprintf(stderr, "message mismatch: expected %lu got %lu\n",
                         static_cast<unsigned long>(i),
                         static_cast<unsigned long>(received.MsgID));
            return false;
        }
    }
    return true;
}

} // namespace

int main()
{
    const std::string serverName =
        "SHMDirectIntegration_" + std::to_string(getpid());
    const std::string shmName = "/" + serverName + ".shm";
    bool passed = false;

    const std::string badShmName =
        "/SHMBadLayout_" + std::to_string(getpid());
    const int badFd = shm_open(badShmName.c_str(), O_CREAT | O_RDWR, 0666);
    bool layoutRejected = false;
    if(badFd != -1)
    {
        ftruncate(badFd, 1);
        close(badFd);
        using Region =
            SHMIPC::TSharedRegion<PackMessage, ClientConf>;
        layoutRejected =
            SHMIPC::shm_mmap_existing<Region>(badShmName.c_str()) == nullptr;
        shm_unlink(badShmName.c_str());
    }

    DirectClient unstartedClient("UnstartedClient");
    PackMessage unstartedMessage{};
    const bool unstartedRejected =
        !unstartedClient.Push(unstartedMessage) &&
        !unstartedClient.Pop(unstartedMessage);

    {
        EchoServer server;
        DirectClient firstClient("DirectClientA");
        DirectClient secondClient("DirectClientB");
        server.Start(serverName, 0);
        firstClient.Start(serverName);
        secondClient.Start(serverName);

        const uint64_t firstGenerationBeforeHeartbeat =
            firstClient.ControlGeneration();
        const uint64_t secondGenerationBeforeHeartbeat =
            secondClient.ControlGeneration();
        const bool heartbeatQueued =
            firstClient.QueueControlForTest(
                SHMIPC::EMsgType::EMSG_TYPE_HEARTBEAT);
        const auto heartbeatDeadline =
            std::chrono::steady_clock::now() +
            std::chrono::milliseconds(500);
        bool heartbeatProcessed = false;
        while(std::chrono::steady_clock::now() < heartbeatDeadline)
        {
            if(server.HeartbeatCount() != 0)
            {
                heartbeatProcessed = true;
                break;
            }
            __builtin_ia32_pause();
        }

        const bool generationsIndependent =
            firstClient.ControlGeneration() >
                firstGenerationBeforeHeartbeat &&
            secondClient.ControlGeneration() ==
                secondGenerationBeforeHeartbeat;

        if(!heartbeatQueued || !heartbeatProcessed ||
           !generationsIndependent ||
           !firstClient.IsConnected() ||
           !secondClient.IsConnected())
        {
            std::fprintf(stderr, "control path or channel connection failed\n");
        }
        else
        {
            passed = RunRoundTrips(firstClient, 500) &&
                     RunRoundTrips(secondClient, 500);
        }

        firstClient.Stop();
        firstClient.Join();
        secondClient.Stop();
        secondClient.Join();

        const uint64_t ackCountBeforeSaturation =
            server.ClientAckCount();
        DirectClient saturationClient("DirectControlFull");
        saturationClient.Start(serverName);
        const auto ackDeadline =
            std::chrono::steady_clock::now() +
            std::chrono::milliseconds(500);
        while(server.ClientAckCount() == ackCountBeforeSaturation &&
              std::chrono::steady_clock::now() < ackDeadline)
        {
            __builtin_ia32_pause();
        }
        const bool saturationConnected =
            server.ClientAckCount() > ackCountBeforeSaturation;
        const uint64_t generationBeforeFailure =
            saturationClient.ControlGeneration();
        const bool queueFilled = saturationClient.FillControlQueueForTest();
        const bool rejected = !saturationClient.QueueControlForTest(
            SHMIPC::EMsgType::EMSG_TYPE_HEARTBEAT);
        const bool generationUnchanged =
            saturationClient.ControlGeneration() ==
            generationBeforeFailure;
        saturationClient.Stop();
        saturationClient.Join();
        passed = passed && unstartedRejected &&
                 saturationConnected && queueFilled && rejected &&
                 generationUnchanged;

        if(passed && layoutRejected)
        {
            DirectClient reconnectedClient("DirectClientA");
            reconnectedClient.Start(serverName);
            passed = reconnectedClient.IsConnected() &&
                     RunRoundTrips(reconnectedClient, 100);
            reconnectedClient.Stop();
            reconnectedClient.Join();
        }
        server.Stop();
        server.Join();
    }

    shm_unlink(shmName.c_str());
    std::printf("SHM direct IPC integration test %s\n",
                passed ? "passed" : "failed");
    return passed ? 0 : 1;
}
