#include "Common.hpp"
#include "SHMConnection.hpp"

#include <time.h>
#include <sched.h>
#include <vector>
#include <string>
#include <thread>
#include <algorithm>
#include <numeric>
#include <math.h>

inline unsigned long long GetCurrentTimeNs() 
{
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

struct PackMessage
{
    uint64_t MsgID;
    char data[100];
    uint64_t TimeStamp;
    uint32_t ChannelID;
};

const uint64_t N = 10000000;

struct ClientConf : public SHMIPC::CommonConf
{
    static const bool Performance = true;
};

class EchoClient 
{
public:
    EchoClient(const std::string& ClientName):m_Client(ClientName)
    {
        m_pWorkThread = nullptr;
        m_ClientName = ClientName;
    }

    virtual ~EchoClient()
    {
    }

    void Start(const std::string& ServerName, int32_t CPU0, int32_t CPU1)
    {
        m_Client.Start(ServerName, CPU0);
        m_CPU = CPU1;
        m_pWorkThread = new std::thread(&EchoClient::WorkFunc, this);
        m_pWorkThread->join();
        m_Client.Join();
    }
protected:
    void WorkFunc()
    {
        bool ret = SHMIPC::ThreadBind(pthread_self(), m_CPU);
        printf("EchoClient %s start WorkFunc thread, CPU:%d ret=%d\n", m_ClientName.c_str(), m_CPU, ret);
        PackMessage sendMsg, recvMsg;
        sendMsg.MsgID = 0;
        recvMsg.MsgID = 0;
        while(recvMsg.MsgID < N)
        {
            if(m_Client.Push(sendMsg))
            {
                sendMsg.MsgID++;
            }
            if(recvMsg.MsgID < N && m_Client.Pop(recvMsg))
            {
                recvMsg.MsgID++;
            }
        }
        m_Client.Stop();
        printf("EchoClient %s execute WorkFunc thread done\n", m_ClientName.c_str());
    }
private:
    SHMIPC::SHMConnection<PackMessage, ClientConf> m_Client;
    std::thread* m_pWorkThread;
    std::string m_ClientName;
    int32_t m_CPU;
};


int main(int argc, char* argv[]) 
{
    if(argc < 5)
    {
        printf("Usage: %s <ClientName> <ServerName> <cpuid> <cpuid>\n", argv[0]);
        return -1;
    }
    EchoClient client(argv[1]);
    uint64_t start = GetCurrentTimeNs();
    client.Start(argv[2], std::atoi(argv[3]), std::atoi(argv[4]));

    uint64_t end = GetCurrentTimeNs();
    double latency = 1.0 * (end - start) / N;
    printf("recv Msg: %d latency:%.2f ns\n", N, latency);

    return 0;
}
// g++ -std=c++17 -O3 -o EchoClient EchoClient.cpp -lpthread -lrt