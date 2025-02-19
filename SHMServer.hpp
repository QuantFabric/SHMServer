
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
        m_MsgID = 1;
        m_Stopped = false;

        m_pSendQueue = new SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize * 4>;
        m_pRecvQueue = new SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize * 4>;
    }

    virtual ~SHMServer() 
    {
        Stop();
        Release();
    }

    void Start(const std::string& ServerName, int32_t CPUID=-1)
    {
        m_CPUID = CPUID;
        // 初始化共享内存
        InitChannel(ServerName);
        // 注册信号处理函数
        auto signal_handler = [](int signal) {
            if(signal == SIGINT)
            {
                fprintf(stdout, "SHMServer recv SIGINT,Quit...\n");
            }
            else if(signal == SIGTERM)
            {
                fprintf(stdout, "SHMServer recv SIGTERM,Quit...\n");
            }
            else if(signal == SIGKILL)
            {
                fprintf(stdout, "SHMServer recv SIGKILL,Quit...\n");
            }
            fflush(stdout);
            exit(signal);  // 退出程序
        };
        signal(SIGKILL, signal_handler);
        signal(SIGTERM, signal_handler);
        signal(SIGINT, signal_handler);
        // 开启性能模式
        if constexpr (Conf::Performance)
        {
            // 启动线程
            fprintf(stdout, "Start SHMServer InternalThread Publish:%d\n", Conf::Publish);
            fflush(stdout); 
            fflush(stderr);
            m_pInternalThread = new std::thread(&SHMServer::WorkFunc, this);
        }
        else
        {
            WorkFunc();
            fflush(stdout); 
            fflush(stderr);
        }
    }

    bool Push(const T& msg)
    {
        static TChannelMsg<T> Msg;
        Msg.ChannelID = msg.ChannelID;
        Msg.MsgType = EMsgType::EMSG_TYPE_DATA;
        Msg.TimeStamp = RDTSC();
        memcpy(&Msg.Data, &msg, sizeof(Msg.Data));
        return m_pSendQueue->Push(Msg);
    }

    bool Pop(T& msg)
    {
        static TChannelMsg<T> Msg;
        bool ret = m_pRecvQueue->Pop(Msg);
        if(ret && EMsgType::EMSG_TYPE_DATA == Msg.MsgType)
        {
            memcpy(&msg, &Msg.Data, sizeof(T));
            msg.ChannelID = Msg.ChannelID;
            msg.TimeStamp = Msg.TimeStamp;
        }
        return ret;
    }

    void Stop() 
    {
        m_Stopped = true;
    }

    void Join()
    {
        if(m_pInternalThread)
        {
            m_pInternalThread->join();
        }
    }

    virtual void HandleMsg()
    {
       
    }

   void PollMsg() 
    {
        // 发布订阅模式
        if constexpr (Conf::Publish)
        {
            // 发布消息到客户端
            while(true)
            {
                bool ret = m_pSendQueue->Pop(m_Msg);
                if(ret)
                {
                    m_Msg.MsgID = m_MsgID++;
                    for(uint16_t i = 0; i < Conf::ChannelSize; i++) 
                    {
                        if(m_AllChannel[i].IsConnected)
                        {
                            bool ret = m_AllChannel[i].SendQueue.Push(m_Msg);
                            if(ret)
                            {
                                m_AllChannel[i].TimeStamp = RDTSC();
                            }
                            else
                            {
                                fprintf(stderr, "SHMServer Channel:%d ChannelName:%s SendQueue full, misss Msg:%u\n", 
                                        m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName, m_Msg.MsgID);
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
                    if(EMsgType::EMSG_TYPE_LOGIN == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer recv LOGIN from Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                        static TChannelMsg<T> ServerACKMsg;
                        ServerACKMsg.MsgType = EMsgType::EMSG_TYPE_SERVER_ACK;
                        ServerACKMsg.ChannelID = m_AllChannel[i].ChannelID;
                        m_AllChannel[i].SendQueue.Push(ServerACKMsg);
                        fprintf(stdout, "SHMServer send SERVER_ACK to Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                    }
                    else if(EMsgType::EMSG_TYPE_CLIENT_ACK == m_Msg.MsgType)
                    {
                        m_AllChannel[i].IsConnected = true;
                        fprintf(stdout, "SHMServer recv CLIENT_ACK from Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                    }
                    else if(EMsgType::EMSG_TYPE_HEARTBEAT == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer recv HEARTBEAT from Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                    }
                }
                // 超时断开连接
                if(m_AllChannel[i].IsConnected && (m_AllChannel[i].TimeStamp + Conf::Heartbeat_Interval <  RDTSC()))
                {
                    m_AllChannel[i].IsConnected = false;
                    fprintf(stderr, "SHMServer Channel:%d ChannelName:%s disconnect\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                }
            }
            fflush(stdout); 
            fflush(stderr);
        }
        // CS模式
        else
        {
            // 接收客户端消息
            for(uint16_t i = 0; i < Conf::ChannelSize; i++) 
            {
                if(m_AllChannel[i].RecvQueue.Pop(m_Msg))
                {
                    // fprintf(stdout, "SHMServer recv msg from Channnel:%d Msg:%u MsgType:%d\n", m_AllChannel[i].ChannelID, m_Msg.MsgID, m_Msg.MsgType);
                    m_Msg.MsgID = m_MsgID++;
                    m_AllChannel[i].TimeStamp = RDTSC();
                    if(EMsgType::EMSG_TYPE_DATA == m_Msg.MsgType)
                    {
                        bool ret = m_pRecvQueue->Push(m_Msg);
                        if(!ret)
                        {
                            fprintf(stderr, "SHMServer Channel:%d ChannelName:%s m_pRecvQueue full, misss Msg:%u\n", 
                                    m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName, m_Msg.MsgID);
                        }
                        // fprintf(stdout, "SHMServer recv Data from Channnel:%d Msg:%u\n", m_AllChannel[i].ChannelID, m_Msg.MsgID);
                    }
                    else if(EMsgType::EMSG_TYPE_LOGIN == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer recv LOGIN from Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                        TChannelMsg<T> ServerACKMsg;
                        ServerACKMsg.MsgType = EMsgType::EMSG_TYPE_SERVER_ACK;
                        ServerACKMsg.ChannelID = m_AllChannel[i].ChannelID;
                        m_AllChannel[i].SendQueue.Push(ServerACKMsg);
                        fprintf(stdout, "SHMServer send SERVER_ACK to Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                    }
                    else if(EMsgType::EMSG_TYPE_CLIENT_ACK == m_Msg.MsgType)
                    {
                        m_AllChannel[i].IsConnected = true;
                        fprintf(stdout, "SHMServer recv CLIENT_ACK from Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                    }
                    else if(EMsgType::EMSG_TYPE_HEARTBEAT == m_Msg.MsgType)
                    {
                        fprintf(stdout, "SHMServer recv HEARTBEAT from Channel:%d ChannelName:%s\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                    }
                }
                // 超时断开连接
                if(m_AllChannel[i].IsConnected && (m_AllChannel[i].TimeStamp + Conf::Heartbeat_Interval <  RDTSC()))
                {
                    m_AllChannel[i].IsConnected = false;
                    fprintf(stderr, "SHMServer Channel:%d ChannelName:%s disconnect for timeout\n", m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName);
                }
            }
            // 发送消息到客户端
            while(true)
            {
                bool ret = m_pSendQueue->Pop(m_Msg);
                if(ret)
                {
                    m_Msg.MsgID = m_MsgID++;
                    if(m_AllChannel[m_Msg.ChannelID].IsConnected)
                    {
                        bool ret = m_AllChannel[m_Msg.ChannelID].SendQueue.Push(m_Msg);
                        if(ret)
                        {
                            m_AllChannel[m_Msg.ChannelID].TimeStamp = RDTSC();
                            // fprintf(stdout, "SHMServer send Data from Channnel:%d Msg:%u\n", m_AllChannel[m_Msg.ChannelID].ChannelID, m_Msg.MsgID);
                        }
                        else
                        {
                            fprintf(stderr, "SHMServer Channel:%u ChannelName:%s SendQueue full, miss Msg:%u\n", m_Msg.ChannelID, m_AllChannel[m_Msg.ChannelID].ChannelName, m_Msg.MsgID);
                        }
                    }
                }
                else
                {
                    break;
                }
            }
            fflush(stdout); 
            fflush(stderr);
        }
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
                memset(&m_AllChannel[i].ChannelName, 0, sizeof(m_AllChannel[i].ChannelName));
                m_AllChannel[i].ChannelID = i;
                m_AllChannel[i].SendQueue.Reset();
                m_AllChannel[i].RecvQueue.Reset();
                m_AllChannel[i].TimeStamp = RDTSC();
                m_AllChannel[i].IsConnected = false;
                fprintf(stdout, "SHMServer Init Channel:%d vaddr:%p\n", i, &m_AllChannel[i]);
            }
            fprintf(stdout, "SHMServer Init %s done %.2f MB\n", ServerName.c_str(), sizeof(TChannel) * Conf::ChannelSize / 1024.0 / 1024.0);
        }
        else
        {
            fprintf(stderr, "SHMServer %s Init Channel failed\n", ServerName.c_str());
        }
        fflush(stdout); 
        fflush(stderr);
    }

    void Release()
    {
        if(m_AllChannel)
        {
            shm_munmap<TChannel>(m_AllChannel, Conf::ChannelSize);
            m_AllChannel = nullptr;
        }
        if(m_pInternalThread)
        {
            delete m_pInternalThread;
            m_pInternalThread = nullptr;
        }
    }

    void WorkFunc()
    {
        // 开启性能模式
        if constexpr (Conf::Performance)
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
        else
        {
            PollMsg();
            HandleMsg();
        }
    }
protected:
    using SHMQ = SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize>;
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
    volatile bool m_Stopped;
    std::thread* m_pInternalThread = nullptr;
    TChannelMsg<T> m_Msg;
    int32_t m_CPUID;
    uint64_t m_MsgID;
    SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize * 4>* m_pSendQueue;
    SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize * 4>* m_pRecvQueue;
};

// template <class T, class Conf> 
// volatile bool SHMServer<T, Conf>::m_Stopped = false;
// template <class T, class Conf> 
// SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize * 4> SHMServer<T, Conf>::m_pSendQueue;
// template <class T, class Conf> 
// SPSCQueue<TChannelMsg<T>, Conf::ShmQueueSize * 4> SHMServer<T, Conf>::m_pRecvQueue;

} // namespace SHMIPC

#endif // SHMSERVER_HPP