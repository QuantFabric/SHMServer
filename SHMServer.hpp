
#ifndef SHMSERVER_HPP
#define SHMSERVER_HPP

#include <string>
#include <thread>
#include <vector>
#include <strings.h>
#include <signal.h>
#include "SHMConnection.hpp"
#include "Common.hpp"
#include "SPSCQueue.hpp"

namespace SHMIPC {

template<class T, class Conf>
class SHMServer
{
public:
    SHMServer()
    {
        m_AllChannel = nullptr;
        m_pInternalThread = nullptr;
    }

    virtual ~SHMServer() 
    {
        Stop();
    }

    void Start(const std::string& ServerName, int32_t CPUID=-1)
    {
        m_CPUID = CPUID;
        // 初始化共享内存
        InitChannel(ServerName);
        // 注册信号
        signal(SIGTERM, SHMServer::SignalHandler);
        // 启动线程
        fprintf(stdout, "Start SHMServer InternalThread Publish:%d\n", Conf::Publish);
        m_pInternalThread = new std::thread(&SHMServer::WorkFunc, this);
        
    }

    void Join()
    {
        if(m_pInternalThread)
        {
            m_pInternalThread->join();
        }
    }

    bool Push(const ChannelMsg<T>& Msg)
    {
        return m_SendQueue.Push(Msg);
    }

    bool Pop(ChannelMsg<T>& Msg)
    {
        return m_RecvQueue.Pop(Msg);
    }

    void Stop() 
    {
        m_Stopped = true;
        Release();
    }
protected:
    void InitChannel(const std::string& ServerName)
    {
        std::string shm_file = std::string("/") + ServerName + ".shm";
        m_AllChannel = shm_mmap<TChannel>(shm_file.c_str(), Conf::ChannelSize);
        if(m_AllChannel) 
        {
            for(int i = 0; i < Conf::ChannelSize; i++)
            {
                m_AllChannel[i].ChannelID = i;
                m_AllChannel[i].SendQueue.Reset();
                m_AllChannel[i].RecvQueue.Reset();
                m_AllChannel[i].TimeStamp = RDTSC();
                fprintf(stdout, "SHMServer Init Channel:%d 0X%p\n", i, &m_AllChannel[i]);
            }
            fprintf(stdout, "SHMServer Init %s done %.2f MB\n", ServerName.c_str(), sizeof(TChannel) * Conf::ChannelSize / 1024.0 / 1024.0);
        }
        else
        {
            fprintf(stderr, "SHMServer %s Init Channel failed\n", ServerName.c_str());
        }
    }

    void Release()
    {
        if(m_AllChannel)
        {
            shm_munmap<TChannel>(m_AllChannel, Conf::ChannelSize);
            m_AllChannel = nullptr;
        }
    }

    static void SignalHandler(int s) 
    {
        m_Stopped = true;
    }

    void WorkFunc()
    {
        // 绑定CPU
        bool ret = SHMIPC::ThreadBind(pthread_self(), m_CPUID);
        fprintf(stdout, "SHMServer::WorkFunc InternalThread bind to CPU:%d, ret=%d\n", m_CPUID, ret);
        while(!m_Stopped) 
        {
            PollMsg();
            HandleMsg();
        }
        // 停止服务
        Stop();
    }

    void PollMsg() 
    {
        // 发布订阅模式
        if constexpr (Conf::Publish)
        {
            // 发布消息到客户端
            while(true)
            {
                bool ret = m_SendQueue.Pop(m_Msg);
                if(ret)
                {
                    for(uint16_t i = 0; i < Conf::ChannelSize; i++) 
                    {
                        if(m_AllChannel[i].IsConnected)
                        {
                            // while(!m_AllChannel[i].SendQueue.Push(m_Msg));
                            bool ret = m_AllChannel[i].SendQueue.Push(m_Msg);
                            if(!ret)
                            {
                                fprintf(stderr, "SHMServer Channel:%u SendQueue full, misss Msg:%u\n", m_AllChannel[i].ChannelID, m_Msg.MsgID);
                            }
                        }
                    }
                }
                else
                {
                    break;
                }
            }
            // 接收客户端的控制类型消息
            for(uint16_t i = 0; i < Conf::ChannelSize; i++) 
            {
                if(m_AllChannel[i].RecvQueue.Pop(m_Msg))
                {
                    m_AllChannel[i].TimeStamp = RDTSC();
                    if(MsgType::MSG_TYPE_LOGIN == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer recv LOGIN from Channnel:%d\n", m_AllChannel[i].ChannelID);
                        ChannelMsg<T> ServerACKMsg;
                        ServerACKMsg.MsgType = MsgType::MSG_TYPE_SERVER_ACK;
                        ServerACKMsg.ChannelID = m_AllChannel[i].ChannelID;
                        m_AllChannel[i].SendQueue.Push(ServerACKMsg);
                        fprintf(stdout, "SHMServer send Server ACK to Channnel:%d\n", m_AllChannel[i].ChannelID);
                    }
                    else if(MsgType::MSG_TYPE_CLIENT_ACK == m_Msg.MsgType)
                    {
                        m_AllChannel[i].IsConnected = true;
                        fprintf(stdout, "SHMServer recv ACK from Channnel:%d\n", m_AllChannel[i].ChannelID);
                    }
                    else if(MsgType::MSG_TYPE_HEARTBEAT == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer HeartBeat from Channnel:%d\n", m_AllChannel[i].ChannelID);
                    }
                }
                else
                {
                    // 超时断开连接
                    if(m_AllChannel[i].IsConnected && (m_AllChannel[i].TimeStamp + Conf::Heartbeat_Interval <  RDTSC()))
                    {
                        m_AllChannel[i].IsConnected = false;
                        fprintf(stderr, "SHMServer Channnel:%d disconnect\n", m_AllChannel[i].ChannelID);
                    }
                }
            }
        }
        // CS模式
        else
        {
            // 接收客户端消息
            for(uint16_t i = 0; i < Conf::ChannelSize; i++) 
            {
                if(m_AllChannel[i].RecvQueue.Pop(m_Msg))
                {
                    m_AllChannel[i].TimeStamp = RDTSC();
                    if(MsgType::MSG_TYPE_DATA == m_Msg.MsgType)
                    {
                        while(!m_RecvQueue.Push(m_Msg));
                        // bool ret = m_RecvQueue.Push(m_Msg);
                        // if(!ret)
                        // {
                        //     fprintf(stderr, "SHMServer m_RecvQueue full, misss Msg:%u\n", m_Msg.MsgID);
                        // }
                        // fprintf(stdout, "SHMServer recv Data from Channnel:%d\n", m_AllChannel[i].ChannelID);
                    }
                    else if(MsgType::MSG_TYPE_LOGIN == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer recv LOGIN from Channnel:%d Connnection\n", m_AllChannel[i].ChannelID);
                        ChannelMsg<T> ServerACKMsg;
                        ServerACKMsg.MsgType = MsgType::MSG_TYPE_SERVER_ACK;
                        ServerACKMsg.ChannelID = m_AllChannel[i].ChannelID;
                        m_AllChannel[i].SendQueue.Push(ServerACKMsg);
                        fprintf(stdout, "SHMServer send Server ACK to Channnel:%d Connnection\n", m_AllChannel[i].ChannelID);
                    }
                    else if(MsgType::MSG_TYPE_CLIENT_ACK == m_Msg.MsgType)
                    {
                        m_AllChannel[i].IsConnected = true;
                        fprintf(stdout, "SHMServer recv ACK from Channnel:%d Connnection\n", m_AllChannel[i].ChannelID);
                    }
                    else if(MsgType::MSG_TYPE_HEARTBEAT == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer HeartBeat from Channnel:%d Connnection\n", m_AllChannel[i].ChannelID);
                    }
                }
                else
                {
                    // 超时断开连接
                    if(m_AllChannel[i].IsConnected && (m_AllChannel[i].TimeStamp + Conf::Heartbeat_Interval <  RDTSC()))
                    {
                        m_AllChannel[i].IsConnected = false;
                        fprintf(stderr, "SHMServer Channnel:%d Connnection disconnect for timeout\n", m_AllChannel[i].ChannelID);
                    }
                }
            }
            // 发送消息到客户端
            while(true)
            {
                bool ret = m_SendQueue.Pop(m_Msg);
                if(ret)
                {
                    if(m_AllChannel[m_Msg.ChannelID].IsConnected)
                    {
                        while(!m_AllChannel[m_Msg.ChannelID].SendQueue.Push(m_Msg));
                        // bool ret = m_AllChannel[m_Msg.ChannelID].SendQueue.Push(m_Msg);
                        // if(!ret)
                        // {
                        //     fprintf(stderr, "SHMServer Channel:%u SendQueue full, misss Msg:%u\n", m_Msg.ChannelID, m_Msg.MsgID);
                        // }
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }

    virtual void HandleMsg() = 0;

protected:
    using SHMQ = SPSCQueue<ChannelMsg<T>, Conf::ShmQueueSize>;
    typedef struct
    {
        char ChannelName[Conf::NameSize];
        int32_t ChannelID = -1;
        bool IsConnected = false;
        uint64_t TimeStamp;
        alignas(128) SHMQ SendQueue;
        alignas(128) SHMQ RecvQueue;
    }TChannel;
    TChannel* m_AllChannel;
    static volatile bool m_Stopped;
    std::thread* m_pInternalThread = nullptr;
    ChannelMsg<T> m_Msg;
    int32_t m_CPUID;
public:
    static SPSCQueue<ChannelMsg<T>, Conf::ShmQueueSize * 4> m_SendQueue;
    static SPSCQueue<ChannelMsg<T>, Conf::ShmQueueSize * 4> m_RecvQueue;
};

template <class T, class Conf> 
volatile bool SHMServer<T, Conf>::m_Stopped = false;
template <class T, class Conf> 
SPSCQueue<ChannelMsg<T>, Conf::ShmQueueSize * 4> SHMServer<T, Conf>::m_SendQueue;
template <class T, class Conf> 
SPSCQueue<ChannelMsg<T>, Conf::ShmQueueSize * 4> SHMServer<T, Conf>::m_RecvQueue;

} // namespace SHMIPC

#endif // SHMSERVER_HPP