#include "../Common.hpp"
#include "../SHMConnection.hpp"

#include <time.h>
#include <sched.h>
#include <vector>
#include <string>
#include <thread>

struct PackMessage
{
    char data[500];
};


class SubClient 
{
public:
    SubClient(const std::string& ClientName):m_Client(ClientName)
    {
        m_pRecvThread = nullptr;
        m_ChannelID = -1;
        m_ClientName = ClientName;
    }

    virtual ~SubClient()
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
        m_pRecvThread = new std::thread(&SubClient::RecvWorkFunc, this);
        m_pRecvThread->join();
    }
protected:

    void RecvWorkFunc()
    {
        printf("%s start RecvWorkFunc thread, ChannelID:%d\n", m_ClientName.c_str(), m_ChannelID);
        SHMIPC::ChannelMsg<PackMessage> msg;
        while(true)
        {
            if(m_Client.Pop(msg))
            {
                printf("recv Msg: %u\n", msg.MsgID);
            } 
        }
        printf("%s excute RecvWorkFunc done, ChannelID:%d MsgID:%u\n", m_ClientName.c_str(), m_ChannelID, msg.MsgID);
    }
private:
    SHMIPC::SHMConnection<SHMIPC::ChannelMsg<PackMessage>, SHMIPC::CommonConf> m_Client;
    std::thread* m_pRecvThread;
    int32_t m_ChannelID;
    std::string m_ClientName;
};

int main(int argc, char** argv) 
{
    if(argc < 3)
    {
        printf("Usage: %s <ClientName> <ServerName>\n", argv[0]);
        return -1;
    }
    
    SubClient client(argv[1]);
    client.Run(argv[2]);
    
    return 0;
}
// g++ -std=c++17 -O3 -o SubClient SubClient.cpp -lpthread -lrt