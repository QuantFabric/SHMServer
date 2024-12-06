#include "../Common.hpp"
#include "../SHMConnection.hpp"

#include <time.h>
#include <sched.h>
#include <vector>
#include <string>
#include <thread>

inline unsigned long long GetCurrentTimeNs() 
{
    timespec ts;
    ::clock_gettime(CLOCK_REALTIME, &ts);
    return ts.tv_sec * 1000000000 + ts.tv_nsec;
}

struct PackMessage
{
    char data[1024];
};


const uint64_t N = 10000000;

class EchoClient 
{
public:
    EchoClient(const std::string& ClientName, const std::vector<int>& items):m_Client(ClientName)
    {
        m_pSendThread = nullptr;
        m_pRecvThread = nullptr;
        m_ChannelID = -1;
        m_ClientName = ClientName;
        m_CPU_List = items;
    }

    virtual ~EchoClient()
    {
    }

    void Run(const std::string& ServerName)
    {
        if(!m_Client.Connect(ServerName))
        {
            printf("%s connect to %s failed\n", m_ClientName.c_str(), ServerName.c_str());
            return ;
        }
        m_Client.Reset();
        usleep(1000);
        m_ChannelID = m_Client.ChannelID();
        printf("%s connect to %s success, ChannelID:%d\n", m_ClientName.c_str(), ServerName.c_str(), m_ChannelID);
        m_pSendThread = new std::thread(&EchoClient::SendWorkFunc, this);
        m_pRecvThread = new std::thread(&EchoClient::RecvWorkFunc, this);
        m_pSendThread->join();
        m_pRecvThread->join();
    }
protected:

    void SendWorkFunc()
    {
        bool ret = SHMIPC::ThreadBind(pthread_self(), m_CPU_List.at(0));
        printf("%s start SendWorkFunc thread, ChannelID:%d, CPU:%d ret:%d\n", m_ClientName.c_str(), m_ChannelID, m_CPU_List.at(0), ret);
        int64_t i = 0;
        SHMIPC::ChannelMsg<PackMessage> msg;
        while(i < N)
        {
            msg.MsgID = i;
            msg.ChannelID = m_ChannelID;
            // msg.timestamp = GetCurrentTimeNs();
            if(m_Client.Push(msg))
            {
                i++;
                // printf("send Msg: %05d \n", msg.MsgID);
            }
        }
        printf("%s excute SendWorkFunc done, ChannelID:%d MsdID:%d i=%d\n", m_ClientName.c_str(), m_ChannelID, msg.MsgID, i);
    }

    void RecvWorkFunc()
    {
        bool ret = SHMIPC::ThreadBind(pthread_self(), m_CPU_List.at(1));
        printf("%s start RecvWorkFunc thread, ChannelID:%d CPU:%d ret:%d\n", m_ClientName.c_str(), m_ChannelID, m_CPU_List.at(1), ret);
        uint64_t i = 0;
        SHMIPC::ChannelMsg<PackMessage> msg;
        msg.MsgID = 0;
        while(i < N)
        {
            if(m_Client.Pop(msg))
            {
                // printf("recv Msg: %05d %u\n", msg.MsgID, i);
                i++;
            } 
        }
        printf("%s excute RecvWorkFunc done, ChannelID:%d MsdID:%d i=%d\n", m_ClientName.c_str(), m_ChannelID, msg.MsgID, i);
    }
private:
    SHMIPC::SHMConnection<SHMIPC::ChannelMsg<PackMessage>, SHMIPC::CommonConf> m_Client;
    std::thread* m_pSendThread;
    std::thread* m_pRecvThread;
    int32_t m_ChannelID;
    std::string m_ClientName;
    std::vector<int> m_CPU_List;
};

int main(int argc, char* argv[]) 
{
    if(argc < 5)
    {
        printf("Usage: %s <ClientName> <ServerName> <cpuid> <cpuid>\n", argv[0]);
        return -1;
    }
    
    std::vector<int> items;
    items.push_back(std::stoi(argv[3]));
    items.push_back(std::stoi(argv[4]));
    EchoClient client(argv[1], items);
    uint64_t start = GetCurrentTimeNs();
    client.Run(argv[2]);
    uint64_t end = GetCurrentTimeNs();
    double latency = (end - start) / N;
    printf("recv Msg: %d latency:%.2f ns\n", N, latency);
    
    return 0;
}
// g++ -std=c++11 -O3 -o EchoClient EchoClient.cpp -lpthread -lrt