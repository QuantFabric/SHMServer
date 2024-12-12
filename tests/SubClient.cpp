#include "../Common.hpp"
#include "../SHMConnection.hpp"

#include <time.h>
#include <sched.h>
#include <vector>
#include <string>
#include <thread>

struct PackMessage
{
    uint64_t MsgID;
    char data[100];
    uint64_t TimeStamp;
};


int main(int argc, char** argv) 
{
    if(argc < 3)
    {
        printf("Usage: %s <ClientName> <ServerName>\n", argv[0]);
        return -1;
    }

    SHMIPC::SHMConnection<PackMessage, SHMIPC::CommonConf> client(argv[1]);
    client.Start(argv[2]);
    PackMessage recvMsg;
    while(true)
    {
        if(client.Pop(recvMsg))
        {
            printf("recv msg %u\n", recvMsg.MsgID);
        }
        // usleep(1000*1000);
    }
    
    return 0;
}
// g++ -std=c++17 -O3 -o SubClient SubClient.cpp -lpthread -lrt