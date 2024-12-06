#include "../SHMServer.hpp"
#include "../Common.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = false;
};


struct PackMessage
{
    char data[500];
};



class EchoServer : public SHMIPC::SHMServer<SHMIPC::ChannelMsg<PackMessage>, ServerConf>
{
public:
    EchoServer(const std::string& ServerName, const std::vector<int>& items):SHMServer<SHMIPC::ChannelMsg<PackMessage>, ServerConf>(ServerName, items)
    {

    }

    virtual ~EchoServer()
    {

    }

    void WorkFunc()
    {
        SHMIPC::ChannelMsg<PackMessage> msg;
        while(true)
        {
            // 将接收客户端消息发送回客户端
            if(m_RecvQueue.Pop(msg))
            {
                bool ret = m_SendQueue.Push(msg);
                if(!ret)
                {
                    printf("EchoServer m_SendQueue full, misss Msg:%u\n", msg.MsgID);
                }
            }
        }
    }

};


int main(int argc, char* argv[]) 
{
    if(argc < 4)
    {
        printf("Usage: %s <ServerName>\n", argv[0]);
        return -1;
    }
    
    std::vector<int> items;
    items.push_back(std::stoi(argv[2]));
    items.push_back(std::stoi(argv[3]));
    EchoServer server(argv[1], items);
    server.Run();
    return 0;
}
// g++ -std=c++17 -O3 -o EchoServer EchoServer.cpp -lpthread -lrt