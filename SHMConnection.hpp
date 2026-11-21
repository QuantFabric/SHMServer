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
        m_SharedRegion = nullptr;
        strncpy(m_ClientName, clientName.c_str(), sizeof(m_ClientName) - 1);
        m_ClientName[sizeof(m_ClientName) - 1] = 0;

        m_pHeartbeatThread = nullptr;
        m_ChannelID = -1;
        m_LoginMsg.MsgType = EMsgType::EMSG_TYPE_LOGIN;
        m_HeartBeatMsg.MsgType = EMsgType::EMSG_TYPE_HEARTBEAT;

        m_Stopped.store(false, std::memory_order_relaxed);
        m_IsConnected.store(false, std::memory_order_relaxed);
        m_TimeStamp.store(0, std::memory_order_relaxed);

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

    void Start(const std::string& ServerName)
    {
        m_ServerName = ServerName;
        if(!Connect(m_ServerName))
        {
            fprintf(stderr, "SHMConnection::Start %s connect to %s failed\n", m_ClientName, m_ServerName.c_str());
            fflush(stdout);
            fflush(stderr);
            exit(-1);
        }
        fprintf(stdout, "SHMConnection::Start %s connect to %s success by ChannelID:%d\n", m_ClientName, ServerName.c_str(), m_ChannelID);
        m_LoginMsg.ChannelID = m_ChannelID;
        m_HeartBeatMsg.ChannelID = m_ChannelID;
        Reset();
        // 发送登录请求
        const bool loginSent = SendControlMsg(m_LoginMsg);
        fprintf(stdout, "SHMConnection::Start %s send LOGIN to %s by ChannelID:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
        if(!loginSent)
        {
            m_IsConnected.store(false, std::memory_order_release);
        }
        else
        {
            const uint64_t connectStart = RDTSC();
            while(true)
            {
                TControlMsg control{};
                if(m_Channel->ControlSendQueue.Pop(control))
                {
                    if(EMsgType::EMSG_TYPE_SERVER_ACK == control.MsgType)
                    {
                        fprintf(stdout, "SHMConnection::Start %s recv SERVER_ACK from %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
                        control.MsgType = EMsgType::EMSG_TYPE_CLIENT_ACK;
                        control.ChannelID = m_ChannelID;
                        if(!SendControlMsg(control))
                        {
                            m_IsConnected.store(false, std::memory_order_release);
                            break;
                        }
                        m_IsConnected.store(true, std::memory_order_release);
                        fprintf(stdout, "SHMConnection::Start %s send CLIENT_ACK to %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
                        break;
                    }
                }
                if(connectStart + 1e10 < RDTSC())
                {
                    m_IsConnected.store(false, std::memory_order_release);
                    break;
                }
                usleep(1000);
            }
        }
        if(!m_IsConnected.load(std::memory_order_acquire))
        {
            fprintf(stderr, "SHMConnection::Start %s Connected to %s by Channel:%d failed\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
            fflush(stdout);
            fflush(stderr);
            exit(-1);
        }
        fprintf(stdout, "SHMConnection::Start %s Connected to %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
        fflush(stdout); 
        fflush(stderr);
        m_TimeStamp.store(RDTSC(), std::memory_order_relaxed);
        m_pHeartbeatThread = new std::thread(&SHMConnection::HeartbeatThreadFunc, this);
    }

    bool Push(const T& msg)
    {
        if(!m_Channel || !m_IsConnected.load(std::memory_order_acquire))
        {
            return false;
        }

        TDataMsg<T> dataMsg;
        dataMsg.MsgType = EMsgType::EMSG_TYPE_DATA;
        dataMsg.MsgID = 0;
        dataMsg.TimeStamp = RDTSC();
        dataMsg.ChannelID = m_ChannelID;
        memcpy(&dataMsg.Data, &msg, sizeof(T));
        const bool ret = m_Channel->RecvQueue.Push(dataMsg);
        if(ret)
        {
            m_TimeStamp.store(dataMsg.TimeStamp, std::memory_order_relaxed);
            // fprintf(stdout, "SHMConnection::Push %s send msg to %s by Channel:%d\n", m_ClientName, m_ServerName.c_str(), m_ChannelID);
        }
        else
        {
            fprintf(stderr, "SHMConnection::Push %s send msg to %s failed by Channel:%d msg:%lu\n",
                    m_ClientName, m_ServerName.c_str(), m_ChannelID, static_cast<unsigned long>(dataMsg.MsgID));
        }
        return ret;
    }

    bool Pop(T& msg) 
    {
        if(!m_Channel || !m_IsConnected.load(std::memory_order_acquire))
        {
            return false;
        }

        TDataMsg<T> dataMsg;
        if(!m_Channel->SendQueue.Pop(dataMsg))
        {
            return false;
        }
        if(EMsgType::EMSG_TYPE_DATA != dataMsg.MsgType)
        {
            return false;
        }
        memcpy(&msg, &dataMsg.Data, sizeof(T));
        msg.ChannelID = dataMsg.ChannelID;
        msg.TimeStamp = dataMsg.TimeStamp;
        m_TimeStamp.store(dataMsg.TimeStamp, std::memory_order_relaxed);
        return true;
    }

    void Join()
    {
        if(m_pHeartbeatThread)
        {
            m_pHeartbeatThread->join();
        }
    }

    void Stop()
    {
        m_Stopped.store(true, std::memory_order_release);
    }

    bool IsConnected() const
    {
        return m_IsConnected.load(std::memory_order_acquire);
    }

protected:
    void HandleControlMsg()
    {
        // 接收控制消息
        TControlMsg control{};
        if(m_Channel->ControlSendQueue.Pop(control))
        {
            m_TimeStamp.store(RDTSC(), std::memory_order_relaxed);
        }
        // 超时发送心跳包
        const uint64_t timestamp = m_TimeStamp.load(std::memory_order_relaxed);
        if(timestamp + Conf::Heartbeat_Interval - 50e9 < RDTSC())
        {
            const uint64_t heartbeatTimestamp = RDTSC();
            m_TimeStamp.store(heartbeatTimestamp, std::memory_order_relaxed);
            m_HeartBeatMsg.TimeStamp = heartbeatTimestamp;
            SendControlMsg(m_HeartBeatMsg);
            fprintf(stdout, "SHMConnection::HandleControlMsg %s send HEARTBEAT to %s by Channel:%d %lu\n", m_ClientName, m_ServerName.c_str(), m_ChannelID, heartbeatTimestamp);
            fflush(stdout);
        }
    }

    bool Connect(const std::string& ServerName)
    {
        if(m_AllChannel)
        {
            return false;
        }

        std::string shm_file = std::string("/") + ServerName + ".shm";
        m_SharedRegion = shm_mmap_existing<SharedRegion>(shm_file.c_str());
        if(!m_SharedRegion)
        {
            return false;
        }

        if(m_SharedRegion->Version.load(std::memory_order_acquire) != SharedRegion::CurrentVersion)
        {
            fprintf(stderr, "SHMConnection::Connect %s layout version mismatch %#X, rebuild server and client\n", m_ClientName, SharedRegion::CurrentVersion);
            shm_munmap<SharedRegion>(m_SharedRegion, 1);
            m_SharedRegion = nullptr;
            return false;
        }

        m_AllChannel = m_SharedRegion->Channels;
        bool ret = false;
        for(uint16_t i = 0; i < SharedRegion::ChannelCapacity; i++)
        {
            if(strnlen(m_AllChannel[i].ChannelName, sizeof(m_AllChannel[i].ChannelName)) == 0)
            {
                m_Channel = &m_AllChannel[i];
                strncpy(m_Channel->ChannelName, m_ClientName, sizeof(m_Channel->ChannelName) - 1);
                m_Channel->SendQueue.Reset();
                m_Channel->RecvQueue.Reset();
                m_Channel->ControlSendQueue.Reset();
                m_Channel->ControlRecvQueue.Reset();
                m_ChannelID = m_Channel->ChannelID;
                ret = true;
                fprintf(stdout, "SHMConnection::Connect %s new Channel:%d vaddr:%p success, ClientName: %s\n",
                        ServerName.c_str(), i, static_cast<const void*>(m_Channel), m_ClientName);
                break;
            }
            else if(strncmp(m_AllChannel[i].ChannelName, m_ClientName, sizeof(m_AllChannel[i].ChannelName)) == 0)
            {
                m_Channel = &m_AllChannel[i];
                m_Channel->SendQueue.Reset();
                m_Channel->RecvQueue.Reset();
                m_Channel->ControlSendQueue.Reset();
                m_Channel->ControlRecvQueue.Reset();
                m_ChannelID = m_Channel->ChannelID;
                ret = true;
                fprintf(stdout, "SHMConnection::Connect %s exist Channel:%d vaddr:%p success, ClientName: %s\n",
                        ServerName.c_str(), i, static_cast<const void*>(m_Channel), m_ClientName);
                break;
            }
        }
        if(ret)
        {
            fprintf(stdout, "SHMConnection::Connect %s connect to %s success, %.2f MB\n",
                    m_ClientName, ServerName.c_str(), sizeof(SharedRegion) / 1024.0 / 1024.0);
        }
        else
        {
            fprintf(stderr, "SHMConnection::Connect %s connect to %s failed\n", m_ClientName, ServerName.c_str());
        }
        return ret;
    }


    void HeartbeatThreadFunc()
    {
        while(!m_Stopped.load(std::memory_order_acquire))
        {
            HandleControlMsg();
        }
    }

    void Release() 
    {
        if(m_SharedRegion)
        {
            shm_munmap<SharedRegion>(m_SharedRegion, 1);
            m_SharedRegion = nullptr;
            m_AllChannel = nullptr;
            m_Channel = nullptr;
        }
        if(m_pHeartbeatThread)
        {
            delete m_pHeartbeatThread;
            m_pHeartbeatThread = nullptr;
        }
    }

    void Reset() 
    {
        if(m_Channel) 
        {
            m_Channel->SendQueue.Reset();
            m_Channel->RecvQueue.Reset();
            m_Channel->ControlSendQueue.Reset();
            m_Channel->ControlRecvQueue.Reset();
        }
    }

    bool SendControlMsg(const TControlMsg& control)
    {
        if(!m_Channel || !m_Channel->ControlRecvQueue.Push(control))
        {
            return false;
        }
        m_Channel->ControlGeneration.fetch_add(1, std::memory_order_release);
        return true;
    }

protected:
    char m_ClientName[Conf::NameSize];
    int32_t m_ChannelID;
    std::string m_ServerName;
    using SharedRegion = TSharedRegion<T, Conf>;
    using SharedChannel = TSharedChannel<T, Conf>;
    SharedRegion* m_SharedRegion;
    SharedChannel* m_Channel;
    SharedChannel* m_AllChannel;

    std::thread* m_pHeartbeatThread;
    TControlMsg m_LoginMsg{};
    TControlMsg m_HeartBeatMsg{};
    std::atomic<bool> m_Stopped;
    std::atomic<bool> m_IsConnected;
    std::atomic<uint64_t> m_TimeStamp;
};


} // namespace SHMIPC

#endif // SHMCONNECTION_HPP
