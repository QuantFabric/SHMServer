#include "../SHMServer.hpp"
#include "../Common.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = true;
};


struct PackMessage
{
    uint64_t MsgID;
    char data[100];
    uint64_t TimeStamp;
};



class PubServer : public SHMIPC::SHMServer<PackMessage, ServerConf>
{
public:
    PubServer():SHMServer<PackMessage, ServerConf>()
    {

    }

    virtual ~PubServer()
    {

    }

    void HandleMsg()
    {
        usleep(500*1000);// 间隔500ms
        static SHMIPC::ChannelMsg<PackMessage> msg;
        static uint64_t i = 0;
        // 模拟接收行情数据
        for(int j = 0; j < 100; j++)
        {
            msg.Data.MsgID = i++;
            while(!m_SendQueue.Push(msg));
        }
    }
};


int main(int argc, char* argv[]) 
{
    if(argc < 2)
    {
        printf("Usage: %s <ServerName>\n", argv[0]);
        return -1;
    }
    
    PubServer server;
    server.Start(argv[1]);
    server.Join();
    return 0;
}

// g++ -std=c++17 -O3 -o PubServer PubServer.cpp -lpthread -lrt