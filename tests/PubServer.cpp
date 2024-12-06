#include "../SHMServer.hpp"
#include "../Common.hpp"

struct ServerConf : public SHMIPC::CommonConf
{
    static const bool Publish = true;
};


struct PackMessage
{
    char data[1000];
};



class PubServer : public SHMIPC::SHMServer<SHMIPC::ChannelMsg<PackMessage>, ServerConf>
{
public:
    PubServer(const std::string& ServerName, const std::vector<int>& items):SHMServer<SHMIPC::ChannelMsg<PackMessage>, ServerConf>(ServerName, items)
    {

    }

    virtual ~PubServer()
    {

    }

    void WorkFunc()
    {
        SHMIPC::ChannelMsg<PackMessage> msg;
        uint64_t i = 0;
        while(true)
        {   
            // 模拟接收行情数据
            for(int j = 0; j < 100; j++)
            {
                msg.MsgID = i++;
                if(!m_SendQueue.Push(msg))
                {
                    printf("PubServer m_SendQueue full miss Msg:%u\n", msg.MsgID);
                }
            }
            usleep(500*1000);// 间隔500ms
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
    PubServer server(argv[1], items);
    server.Run();
    return 0;
}

// g++ -std=c++17 -O3 -o PubServer PubServer.cpp -lpthread -lrt