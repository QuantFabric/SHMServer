
#ifndef SHMSERVER_HPP
#define SHMSERVER_HPP

#include <array>
#include <atomic>
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
        m_SharedRegion = nullptr;
        m_pInternalThread = nullptr;
        m_MsgID = 1;
        m_Stopped.store(false, std::memory_order_relaxed);

        m_pSendQueue = new SPSCQueue<TDataMsg<T>, Conf::ShmQueueSize * 4>;
        m_pRecvQueue = new SPSCQueue<TDataMsg<T>, Conf::ShmQueueSize * 4>;
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
            fprintf(stdout, "SHMServer::Start InternalThread Publish:%d\n", Conf::Publish);
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
        TDataMsg<T> Msg;
        Msg.ChannelID = msg.ChannelID;
        Msg.MsgType = EMsgType::EMSG_TYPE_DATA;
        Msg.MsgID = 0;
        Msg.TimeStamp = RDTSC();
        memcpy(&Msg.Data, &msg, sizeof(Msg.Data));
        return m_pSendQueue->Push(Msg);
    }

    bool Pop(T& msg)
    {
        TDataMsg<T> Msg;
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
        m_Stopped.store(true, std::memory_order_release);
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
        PollControlMessages();

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
                    for(uint16_t i = 0; i < Conf::PubChannelSize; i++) 
                    {
                        if(m_AllChannel[i].IsConnected.load(std::memory_order_acquire))
                        {
                            bool ret = m_AllChannel[i].SendQueue.Push(m_Msg);
                            if(ret)
                            {
                                m_AllChannel[i].TimeStamp = RDTSC();
                            }
                            else
                            {
                                fprintf(stderr, "SHMServer::PollMsg Channel:%d ChannelName:%s SendQueue full, misss Msg:%u\n", 
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
                    // fprintf(stdout, "SHMServer::PollMsg recv msg from Channnel:%d Msg:%u MsgType:%d\n", m_AllChannel[i].ChannelID, m_Msg.MsgID, m_Msg.MsgType);
                    m_Msg.MsgID = m_MsgID++;
                    m_AllChannel[i].TimeStamp = RDTSC();
                    if(EMsgType::EMSG_TYPE_DATA == m_Msg.MsgType)
                    {
                        bool ret = m_pRecvQueue->Push(m_Msg);
                        if(!ret)
                        {
                            fprintf(stderr, "SHMServer::PollMsg Channel:%d ChannelName:%s m_pRecvQueue full, misss Msg:%u\n", 
                                    m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName, m_Msg.MsgID);
                        }
                        // fprintf(stdout, "SHMSHMServer::PollMsgServer recv Data from Channnel:%d Msg:%u\n", m_AllChannel[i].ChannelID, m_Msg.MsgID);
                    }
                }
            }
            // 发送消息到客户端
            while(true)
            {
                bool ret = m_pSendQueue->Pop(m_Msg);
                if(ret)
                {
                    m_Msg.MsgID = m_MsgID++;
                    if(m_AllChannel[m_Msg.ChannelID].IsConnected.load(std::memory_order_acquire))
                    {
                        bool ret = m_AllChannel[m_Msg.ChannelID].SendQueue.Push(m_Msg);
                        if(ret)
                        {
                            m_AllChannel[m_Msg.ChannelID].TimeStamp = RDTSC();
                            // fprintf(stdout, "SHMServer::PollMsg send Data from Channnel:%d Msg:%u\n", m_AllChannel[m_Msg.ChannelID].ChannelID, m_Msg.MsgID);
                        }
                        else
                        {
                            fprintf(stderr, "SHMServer::PollMsg Channel:%u ChannelName:%s SendQueue full, miss Msg:%u\n", m_Msg.ChannelID, m_AllChannel[m_Msg.ChannelID].ChannelName, m_Msg.MsgID);
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
    void PollControlMessages()
    {
        if(!m_SharedRegion)
        {
            return;
        }

        for(uint16_t i = 0; i < SharedRegion::ChannelCapacity; i++)
        {
            const uint64_t generation = m_AllChannel[i].ControlGeneration.load(std::memory_order_acquire);
            if(generation != m_LastControlGeneration[i])
            {
                m_LastControlGeneration[i] = generation;
                TControlMsg control{};
                while(m_AllChannel[i].ControlRecvQueue.Pop(control))
                {
                    m_AllChannel[i].TimeStamp = RDTSC();
                    HandleControlMsg(m_AllChannel[i], control);
                }
            }

            if(m_AllChannel[i].IsConnected.load(std::memory_order_acquire)
                && m_AllChannel[i].TimeStamp + Conf::Heartbeat_Interval < RDTSC())
            {
                m_AllChannel[i].IsConnected.store(false, std::memory_order_release);
                fprintf(stderr, "SHMServer::PollControlMessages Channel:%d ChannelName:%s disconnect%s\n",
                        m_AllChannel[i].ChannelID, m_AllChannel[i].ChannelName, Conf::Publish ? "" : " for timeout");
            }
        }
    }

    virtual void HandleControlMsg(TSharedChannel<T, Conf>& channel, const TControlMsg& control)
    {
        if(EMsgType::EMSG_TYPE_HEARTBEAT == control.MsgType)
        {
            fprintf(stdout, "SHMServer::HandleControlMsg recv HEARTBEAT from Channel:%d ChannelName:%s\n",
                    channel.ChannelID, channel.ChannelName);
        }
        else if(EMsgType::EMSG_TYPE_LOGIN == control.MsgType)
        {
            fprintf(stdout, "SHMServer::HandleControlMsg recv LOGIN from Channel:%d ChannelName:%s\n",
                    channel.ChannelID, channel.ChannelName);
            TControlMsg serverAck{};
            serverAck.MsgType = EMsgType::EMSG_TYPE_SERVER_ACK;
            serverAck.ChannelID = channel.ChannelID;
            channel.ControlSendQueue.Push(serverAck);
            fprintf(stdout, "SHMServer::HandleControlMsg send SERVER_ACK to Channel:%d ChannelName:%s\n",
                    channel.ChannelID, channel.ChannelName);
        }
        else if(EMsgType::EMSG_TYPE_CLIENT_ACK == control.MsgType)
        {
            channel.IsConnected.store(true, std::memory_order_release);
            fprintf(stdout, "SHMServer::HandleControlMsg recv CLIENT_ACK from Channel:%d ChannelName:%s\n",
                    channel.ChannelID, channel.ChannelName);
            Push_Init_Data(channel.ChannelID);
        }
    }

    void InitChannel(const std::string& ServerName)
    {
        std::string shm_file = std::string("/") + ServerName + ".shm";
        m_SharedRegion = shm_mmap<SharedRegion>(shm_file.c_str(), 1);
        if(m_SharedRegion)
        {
            m_AllChannel = m_SharedRegion->Channels;
            m_SharedRegion->Version.store(0, std::memory_order_release);
            for(uint16_t i = 0; i < SharedRegion::ChannelCapacity; i++)
            {
                memset(&m_AllChannel[i].ChannelName, 0, sizeof(m_AllChannel[i].ChannelName));
                m_AllChannel[i].ChannelID = i;
                m_AllChannel[i].SendQueue.Reset();
                m_AllChannel[i].RecvQueue.Reset();
                m_AllChannel[i].ControlSendQueue.Reset();
                m_AllChannel[i].ControlGeneration.store(0, std::memory_order_relaxed);
                m_AllChannel[i].ControlRecvQueue.Reset();
                m_LastControlGeneration[i] = 0;
                m_AllChannel[i].TimeStamp = RDTSC();
                m_AllChannel[i].IsConnected.store(false, std::memory_order_relaxed);
                fprintf(stdout, "SHMServer::InitChannel Init Channel:%d vaddr:%p\n", i, static_cast<const void*>(&m_AllChannel[i]));
            }
            m_SharedRegion->Version.store(SharedRegion::CurrentVersion, std::memory_order_release);
            fprintf(stdout, "SHMServer::InitChannel Init %s done %.2f MB\n", ServerName.c_str(), sizeof(SharedRegion) / 1024.0 / 1024.0);
        }
        else
        {
            fprintf(stderr, "SHMServer::InitChannel %s Init Channel failed\n", ServerName.c_str());
        }
        fflush(stdout); 
        fflush(stderr);
    }

    virtual void Push_Init_Data(uint16_t ChannelID)
    {
        
    }

    void Release()
    {
        if(m_SharedRegion)
        {
            shm_munmap<SharedRegion>(m_SharedRegion, 1);
            m_SharedRegion = nullptr;
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
            while(!m_Stopped.load(std::memory_order_acquire))
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
    using SharedRegion = TSharedRegion<T, Conf>;
    using SharedChannel = TSharedChannel<T, Conf>;
    SharedRegion* m_SharedRegion;
    SharedChannel* m_AllChannel;
    std::atomic<bool> m_Stopped;
    std::array<uint64_t, SharedRegion::ChannelCapacity> m_LastControlGeneration{};
    std::thread* m_pInternalThread = nullptr;
    TDataMsg<T> m_Msg;
    int32_t m_CPUID;
    uint64_t m_MsgID;
    SPSCQueue<TDataMsg<T>, Conf::ShmQueueSize * 4>* m_pSendQueue;
    SPSCQueue<TDataMsg<T>, Conf::ShmQueueSize * 4>* m_pRecvQueue;
};


} // namespace SHMIPC

#endif // SHMSERVER_HPP
