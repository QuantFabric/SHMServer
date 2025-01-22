#ifndef SHMCONNECTION_HPP
#define SHMCONNECTION_HPP

#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <atomic>
#include <signal.h>
#include "Common.hpp"
#include "SPSCQueue.hpp"
#include "MPMCQueue.hpp"

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

        m_SendQueue.Reset();
        m_RecvQueue.Reset();

        m_pWorkThread = nullptr;
        m_ChannelID = -1;
        m_LoginMsg.MsgType = EMsgType::EMSG_TYPE_LOGIN;
        m_HeartBeatMsg.MsgType = EMsgType::EMSG_TYPE_HEARTBEAT;
        m_DataMsg.MsgType = EMsgType::EMSG_TYPE_DATA;

        m_Stopped = false;
        m_IsConnected = false;
        // 注册信号处理函数
        auto signal_handler = [](int signal) {
            if(signal == SIGINT)
            {
                fprintf(stdout, "SHMConnection recv SIGINT,Quit...\n");
            }
            else if(signal == SIGTERM)
            {
                fprintf(stdout, "SHMConnection recv SIGTERM,Quit...\n");
            }
            else if(signal == SIGKILL)
            {
                fprintf(stdout, "SHMConnection recv SIGKILL,Quit...\n");
            }
            fflush(stdout);
            exit(signal);  // 退出程序
        };
        signal(SIGKILL, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);
    }

    virtual ~SHMConnection()
    {
        Stop();
        Release();
    }

    void Start(const std::string& ServerName, int32_t CPUID=-1)
    {
        m_ServerName = ServerName;
        m_CPUID = CPUID;
        if(!Connect(ServerName))
        {
            fprintf(stderr, "SHMConnection::Start %s connect to %s failed\n", m_ClientName, ServerName.c_str());
            return ;
        }
        fprintf(stdout, "SHMConnection::Start %s connect to %s success by ChannelID:%d\n", m_ClientName, ServerName.c_str(), m_ChannelID);
        Reset();
        m_LoginMsg.ChannelID = m_ChannelID;
        m_HeartBeatMsg.ChannelID = m_ChannelID;
        m_DataMsg.ChannelID = m_ChannelID;
        // 发送登录请求
        m_Channel->SendQueue.Push(m_LoginMsg);
        fprintf(stdout, "SHMConnection::Start %s send LOGIN to %s by ChannelID:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
        while(true)
        {
            static uint64_t TimeStamp = RDTSC();
            TChannelMsg<T> Msg;
            if(m_Channel->RecvQueue.Pop(Msg))
            {
                if(EMsgType::EMSG_TYPE_SERVER_ACK == Msg.MsgType)
                {
                    fprintf(stdout, "SHMConnection::Start %s recv SERVER_ACK from %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
                    Msg.MsgType = EMsgType::EMSG_TYPE_CLIENT_ACK;
                    Msg.ChannelID = m_ChannelID;
                    m_Channel->SendQueue.Push(Msg);
                    m_IsConnected = true;
                    fprintf(stdout, "SHMConnection::Start %s send CLIENT_ACK to %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
                    break;
                }
            }
            if(TimeStamp + 1e10 < RDTSC())
            {
                m_IsConnected = false;
                break;
            }
            usleep(1000);
        }
        if(!m_IsConnected)
        {
            fprintf(stderr, "SHMConnection::Start %s Connected to %s by Channel:%d failed\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
            fflush(stdout);
            fflush(stderr);
            return ;
        }
        fprintf(stdout, "SHMConnection::Start %s Connected to %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
        fflush(stdout); 
        fflush(stderr);
        m_TimeStamp = RDTSC();
        // 开启性能模式，开启工作线程
        if constexpr (Conf::Performance)
        {
            m_pWorkThread = new std::thread(&SHMConnection::WorkFunc, this);
        }
        else
        {
            WorkFunc();
        }
    }

    bool Push(const T& msg) 
    {
        return m_SendQueue.Push(msg);
    }

    bool Pop(T& msg) 
    {
        return m_RecvQueue.Pop(msg);
    }

    void Join()
    {
        if(m_pWorkThread)
        {
            m_pWorkThread->join();
        }
    }

    void Stop()
    {
        m_Stopped = true;
    }

    bool IsConnected() const
    {
        return m_IsConnected;
    }

    void HandleMsg()
    {
        // 发送数据
        if(m_SendQueue.Pop(m_DataMsg.Data))
        {
            if(m_Channel->SendQueue.Push(m_DataMsg))
            {
                // 发送消息时更新时间戳
                m_TimeStamp = RDTSC();
                // fprintf(stdout, "SHMConnection::HandleMsg %s send msg to %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
            }
            else
            {
                fprintf(stderr, "SHMConnection::HandleMsg %s send msg to %s failed by Channel:%d msg:%u\n", 
                                m_ClientName, m_ServerName.c_str(), m_ChannelID, m_DataMsg.MsgID);
            }
        }
        // 接收数据
        if(m_Channel->RecvQueue.Pop(m_DataMsg))
        {
            m_TimeStamp = RDTSC();
            if(EMsgType::EMSG_TYPE_DATA == m_DataMsg.MsgType)
            {
                if(!m_RecvQueue.Push(m_DataMsg.Data))
                {
                    fprintf(stderr, "SHMConnection::HandleMsg %s recv msg failed from %s: m_RecvQueue full\n", m_ClientName, m_ServerName.c_str());
                }
            }
        } 
        // 超时发送心跳包
        if(m_TimeStamp + Conf::Heartbeat_Interval - 50e9 < RDTSC())
        {
            m_TimeStamp = RDTSC();
            m_Channel->SendQueue.Push(m_HeartBeatMsg);
            fprintf(stdout, "SHMConnection::HandleMsg %s send HEARTBEAT to %s by Channel:%d %lu\n", m_ClientName, m_ServerName.c_str(), m_ChannelID, m_TimeStamp);
        }
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
                        m_Channel->SendQueue.Reset();
                        m_Channel->RecvQueue.Reset();
                        m_ChannelID = m_Channel->ChannelID;
                        ret = true;
                        fprintf(stdout, "SHMConnection:%s new Channel:%d success, ClientName: %s\n", ServerName.c_str(), i, m_ClientName);
                        break;
                    }
                    else if(strncmp(m_AllChannel[i].ChannelName, m_ClientName, sizeof(m_AllChannel[i].ChannelName)) == 0) 
                    {
                        m_Channel = &m_AllChannel[i];
                        m_Channel->SendQueue.Reset();
                        m_Channel->RecvQueue.Reset();
                        m_ChannelID = m_Channel->ChannelID;
                        ret = true;
                        fprintf(stdout, "SHMConnection:%s exist Channel:%d success, ClientName: %s\n", ServerName.c_str(), i, m_ClientName);
                        break;
                    }
                }
                if(ret)
                {
                    fprintf(stdout, "SHMConnection %s connect to %s success, %.2f MB\n", m_ClientName, ServerName.c_str(), sizeof(TChannel) * Conf::ChannelSize / 1024.0 / 1024.0);
                }
                else
                {
                    fprintf(stderr, "SHMConnection %s connect to %s failed\n", m_ClientName, ServerName.c_str());
                }
            }
        }
        return ret;
    }


    void WorkFunc()
    {
        // 开启性能模式，绑定CPU
        if constexpr (Conf::Performance)
        {
            bool ret = SHMIPC::ThreadBind(pthread_self(), m_CPUID);
            fprintf(stdout, "SHMConnection::WorkFunc bind WorkThread to CPU:%d ret=%d ClientName:%s ChannelID:%d\n", m_CPUID, ret, m_ClientName, m_ChannelID);
            while(!m_Stopped)
            {
                HandleMsg();
            }
        }
        else
        {
            HandleMsg();
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
        if(m_pWorkThread)
        {
            delete m_pWorkThread;
            m_pWorkThread = nullptr;
        }
    }

    void Reset() 
    {
        if(m_Channel) 
        {
            m_Channel->SendQueue.Reset();
            m_Channel->RecvQueue.Reset();
        }
        m_SendQueue.Reset();
        m_RecvQueue.Reset();
    }
public:
    SPSCQueue<T, Conf::ShmQueueSize> m_SendQueue;
    SPSCQueue<T, Conf::ShmQueueSize> m_RecvQueue;
private:
    char m_ClientName[Conf::NameSize];
    int32_t m_ChannelID;
    std::string m_ServerName;
    using SHMQ = SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize>;
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
    TChannelMsg<T> m_LoginMsg;
    TChannelMsg<T> m_HeartBeatMsg;
    TChannelMsg<T> m_DataMsg;
    int32_t m_CPUID;
    volatile bool m_Stopped;
    volatile bool m_IsConnected;
    uint64_t m_TimeStamp;
};


} // namespace SHMIPC

#endif // SHMCONNECTION_HPP