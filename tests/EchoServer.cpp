#include "../SHMServer.hpp"
#include "../Common.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = false;
};


struct PackMessage
{
    uint64_t MsgID;
    char data[100];
    uint64_t TimeStamp;
};

class EchoServer : public SHMIPC::SHMServer<PackMessage, ServerConf>
{
public:
    EchoServer():SHMServer<PackMessage, ServerConf>()
    {

    }

    virtual ~EchoServer()
    {

    }

    void HandleMsg()
    {
        static SHMIPC::ChannelMsg<PackMessage> msg;
        while(true)
        {
            // 将接收客户端消息发送回客户端
            if(m_RecvQueue.Pop(msg))
            {
                while(!m_SendQueue.Push(msg));
            }
            else
            {
                break;
            }
        }
    }
};

int main(int argc, char* argv[]) 
{
    if(argc < 3)
    {
        printf("Usage: %s <ServerName> <cpuid>\n", argv[0]);
        return -1;
    }
    
    EchoServer server;
    server.Start(argv[1], std::stoi(argv[2]));
    server.Join();
    return 0;
}
// g++ -std=c++17 -O3 -o EchoServer EchoServer.cpp -lpthread -lrt