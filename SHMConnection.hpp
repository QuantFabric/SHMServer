#ifndef SHMCONNECTION_HPP
#define SHMCONNECTION_HPP

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>

#include "SPSCQueue.hpp"
#include "Common.hpp"

namespace SHMIPC {


template<class T, class Conf>
class SHMConnection
{
public:
    SHMConnection(const std::string& clientName) 
    {
        m_Channel = nullptr;
        m_AllChannel = nullptr;
        strncpy(m_ClientName, clientName.c_str(), sizeof(m_ClientName) - 1);
        m_ClientName[sizeof(m_ClientName) - 1] = 0;

        m_pWorkThread = nullptr;
        m_ChannelID = -1;
        m_LoginMsg.MsgType = MsgType::MSG_TYPE_LOGIN;
        m_HeartBeatMsg.MsgType = MsgType::MSG_TYPE_HEARTBEAT;
        m_DataMsg.MsgType = MsgType::MSG_TYPE_DATA;
    }

    virtual ~SHMConnection()
    {
        Release();
    }

    void Start(const std::string& ServerName, int32_t CPUID=-1)
    {
        m_CPUID = CPUID;
        if(!Connect(ServerName))
        {
            fprintf(stderr, "SHMConnection::Start %s connect to %s failed\n", m_ClientName, ServerName.c_str());
            return ;
        }
        fprintf(stdout, "SHMConnection::Start %s connect to %s success ChannelID:%d\n", m_ClientName, ServerName.c_str(), m_ChannelID);
        Reset();
        usleep(1000);
        m_LoginMsg.ChannelID = m_ChannelID;
        m_HeartBeatMsg.ChannelID = m_ChannelID;
        m_DataMsg.ChannelID = m_ChannelID;
        // 发送登录请求
        m_Channel->SendQueue.Push(m_LoginMsg);
        fprintf(stderr, "SHMConnection::Start %s send LOGIN to ChannelID:%d\n", m_ClientName, m_ChannelID);
        m_pWorkThread = new std::thread(&SHMConnection::WorkFunc, this);
    }

    bool Push(const T& msg) 
    {
        return m_SendQueue.Push(msg);
    }

    bool Pop(T& msg) 
    {
        return m_RecvQueue.Pop(msg);
    }

protected:
    bool Connect(const std::string& ServerName)
    {
        bool ret = false;
        std::string shm_file = std::string("/") + ServerName + ".shm";
        if(!m_AllChannel) 
        {
            m_AllChannel = shm_mmap<TChannel>(shm_file.c_str(), Conf::ChannelSize);
            if(m_AllChannel) 
            {
                for(int i = 0; i < Conf::ChannelSize; i++)
                {
                    if(strnlen(m_AllChannel[i].ChannelName, sizeof(m_AllChannel[i].ChannelName)) == 0)
                    {
                        m_Channel = &m_AllChannel[i];
                        strncpy(m_Channel->ChannelName, m_ClientName, sizeof(m_Channel->ChannelName) - 1);
                        m_Channel->ChannelID = i;
                        m_Channel->SendQueue.Reset();
                        m_Channel->RecvQueue.Reset();
                        m_ChannelID = i;
                        ret = true;
                        fprintf(stdout, "SHMConnection:%s new Channel:%d success, ClientName: %s\n", ServerName.c_str(), i, m_ClientName);
                        break;
                    }
                    else if(strncmp(m_AllChannel[i].ChannelName, m_ClientName, sizeof(m_AllChannel[i].ChannelName)) == 0) 
                    {
                        m_Channel = &m_AllChannel[i];
                        m_Channel->ChannelID = i;
                        m_Channel->SendQueue.Reset();
                        m_Channel->RecvQueue.Reset();
                        m_ChannelID = i;
                        ret = true;
                        fprintf(stdout, "SHMConnection:%s exist Channel:%d success, ClientName: %s\n", ServerName.c_str(), i, m_ClientName);
                        break;
                    }
                }
                if(ret)
                {
                    fprintf(stdout, "SHMConnection %s connect to SHMServer:%s success, %.2f MB\n", m_ClientName, ServerName.c_str(), sizeof(TChannel) * Conf::ChannelSize / 1024.0 / 1024.0);
                }
                else
                {
                    fprintf(stderr, "SHMConnection %s connect to SHMServer:%s failed\n", m_ClientName, ServerName.c_str());
                }
            }
        }
        return ret;
    }


    void WorkFunc()
    {
        bool ret = SHMIPC::ThreadBind(pthread_self(), m_CPUID);
        fprintf(stdout, "SHMConnection::WorkFunc bind WorkThread to CPU:%d ret=%d ClientName:%s ChannelID:%d\n", m_CPUID, ret, m_ClientName, m_ChannelID);
        ChannelMsg<T> msg;
        uint64_t TimeStamp = RDTSC();
        while(true)
        {
            // 发送数据
            if(m_SendQueue.Pop(m_DataMsg.Data))
            {
                while(!m_Channel->SendQueue.Push(m_DataMsg));
                TimeStamp = RDTSC();
                // if(m_Channel->SendQueue.Push(m_DataMsg))
                // {
                //     // 发送消息时更新时间戳
                //     TimeStamp = RDTSC();
                //     // fprintf(stdout, "SHMConnection::WorkFunc %s send msg to Channel:%d success\n", m_ClientName, m_ChannelID);
                // }
                // else
                // {
                //     fprintf(stderr, "SHMConnection::WorkFunc %s send msg to Channel:%d fail\n", m_ClientName, m_ChannelID);
                // }
            }
            // 接收数据
            if(m_Channel->RecvQueue.Pop(msg))
            {
                if(MsgType::MSG_TYPE_DATA == msg.MsgType)
                {
                    while(!m_RecvQueue.Push(msg.Data));
                    // fprintf(stdout, "SHMConnection::WorkFunc %s recv msg from Channel:%d\n", m_ClientName, m_ChannelID);
                }
                else if(MsgType::MSG_TYPE_SERVER_ACK == msg.MsgType)
                {
                    fprintf(stdout, "SHMConnection::WorkFunc %s recv ACK from SHMServer\n", m_ClientName);
                    msg.MsgType = MsgType::MSG_TYPE_CLIENT_ACK;
                    msg.ChannelID = m_ChannelID;
                    m_Channel->SendQueue.Push(msg);
                    fprintf(stdout, "SHMConnection::WorkFunc %s send ACK to Channel:%d\n", m_ClientName, m_ChannelID);
                }
            } 
            // 超时发送心跳包
            if(TimeStamp + Conf::Heartbeat_Interval - 10e9 < RDTSC())
            {
                TimeStamp = RDTSC();
                m_Channel->SendQueue.Push(m_HeartBeatMsg);
                fprintf(stdout, "SHMConnection::WorkFunc %s send heartbeat to Channel:%d\n", m_ClientName, m_ChannelID);
            }
        }
    }

    void Release() 
    {
        if(m_AllChannel) 
        {
            shm_munmap<TChannel>(m_AllChannel, Conf::ChannelSize);
            m_AllChannel = nullptr;
            m_Channel = nullptr;
        }
    }

    void Reset() 
    {
        if(m_Channel) 
        {
            m_Channel->SendQueue.Reset();
            m_Channel->RecvQueue.Reset();
        }
    }
public:
    static SPSCQueue<T, Conf::ShmQueueSize> m_SendQueue;
    static SPSCQueue<T, Conf::ShmQueueSize> m_RecvQueue;
private:
    char m_ClientName[Conf::NameSize];
    int32_t m_ChannelID;

    using SHMQ = SPSCQueue<ChannelMsg<T>, Conf::ShmQueueSize>;
    typedef struct
    {
        char ChannelName[Conf::NameSize];
        int32_t ChannelID = -1;
        bool IsConnected = false;
        uint64_t TimeStamp;
        alignas(128) SHMQ RecvQueue;
        alignas(128) SHMQ SendQueue;
    }TChannel;
    TChannel* m_Channel;
    TChannel* m_AllChannel;

    std::thread* m_pWorkThread;
    ChannelMsg<T> m_LoginMsg;
    ChannelMsg<T> m_HeartBeatMsg;
    ChannelMsg<T> m_DataMsg;
    int32_t m_CPUID;
};

template <class T, class Conf> 
SPSCQueue<T, Conf::ShmQueueSize> SHMConnection<T, Conf>::m_SendQueue;
template <class T, class Conf> 
SPSCQueue<T, Conf::ShmQueueSize> SHMConnection<T, Conf>::m_RecvQueue;

} // namespace SHMIPC

#endif // SHMCONNECTION_HPP