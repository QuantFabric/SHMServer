#include "SHMServer.hpp"
#include "Common.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = false;
    static const bool Performance = true;
};


struct PackMessage
{
    uint64_t MsgID;
    char data[100];
    uint64_t TimeStamp;
    uint32_t ChannelID;
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
        static SHMIPC::TChannelMsg<PackMessage> Msg;
        while(true)
        {
            // 将接收客户端消息发送回客户端
            if(m_pRecvQueue->Pop(Msg))
            {
                if(!m_pSendQueue->Push(Msg))
                {
                    fprintf(stderr, "EchoServer::HandleMsg send msg failed, m_SendQueue full\n");
                    break;
                }
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