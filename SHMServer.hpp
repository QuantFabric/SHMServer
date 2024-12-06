
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
    SHMServer(const std::string& ServerName, const std::vector<int>& CPU_List)
    {
        m_AllChannel = nullptr;
        m_pInternalThread = nullptr;
        m_pWorkThread = nullptr;
        m_CPU_List = CPU_List;
        // 初始化共享内存
        InitChannel(ServerName);
        // 注册信号
        signal(SIGTERM, SHMServer::SignalHandler);
    }

    virtual ~SHMServer() 
    {
        Stop();
    }

    void Run()
    {
        m_pInternalThread = new std::thread(&SHMServer::HandleMsgFunc, this);
        m_pWorkThread = new std::thread(&SHMServer::WorkThreadFunc, this);
        m_pInternalThread->join();
        m_pWorkThread->join();
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
                fprintf(stdout, "SHMServer Init Channel:%d 0X%p\n", i, &m_AllChannel[i]);
            }
            fprintf(stdout, "SHMServer Init done %.2f MB\n", sizeof(TChannel) * Conf::ChannelSize / 1024.0 / 1024.0);
        }
        else
        {
            fprintf(stderr, "SHMServer Init Channel failed\n");
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

    void HandleMsgFunc()
    {
        // 绑定线程
        bool ret = ThreadBind(pthread_self(), m_CPU_List.at(0));
        fprintf(stdout, "SHMServer InternalThread bind to cpu:%d ret=%d\n", m_CPU_List.at(0), ret);
        while(!m_Stopped) 
        {
            PollMsg();
        }
        // 停止服务
        Stop();
    }

    void WorkThreadFunc()
    {
        // 绑定线程
        bool ret = ThreadBind(pthread_self(), m_CPU_List.at(1));
        fprintf(stdout, "SHMServer WorkThread bind to cpu:%d ret=%d\n", m_CPU_List.at(1), ret);
        WorkFunc();
    }

    virtual void WorkFunc() = 0;

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
                        bool ret = m_AllChannel[i].SendQueue.Push(m_Msg);
                        if(!ret)
                        {
                            fprintf(stderr, "SHMServer Channel:%u SendQueue full, misss Msg:%u\n", m_AllChannel[i].ChannelID, m_Msg.MsgID);
                        }
                    }
                }
                else
                {
                    break;
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
                    bool ret = m_RecvQueue.Push(m_Msg);
                    if(!ret)
                    {
                        fprintf(stderr, "SHMServer m_RecvQueue full, misss Msg:%u\n", m_Msg.MsgID);
                    }
                }
            }
            // 发送消息到客户端
            while(true)
            {
                bool ret = m_SendQueue.Pop(m_Msg);
                if(ret)
                {
                    if(m_Msg.ChannelID > -1 && m_Msg.ChannelID < Conf::ChannelSize)// 消息交互模式
                    {
                        bool ret = m_AllChannel[m_Msg.ChannelID].SendQueue.Push(m_Msg);
                        if(!ret)
                        {
                            fprintf(stderr, "SHMServer Channel:%u SendQueue full, misss Msg:%u\n", m_Msg.ChannelID, m_Msg.MsgID);
                        }
                    }
                }
                else
                {
                    break;
                }
            }
        }
    }
protected:
    using SHMQ = SPSCQueue<T, Conf::ShmQueueSize>;
    typedef struct
    {
        char ChannelName[Conf::NameSize];
        int32_t ChannelID = -1;
        bool IsConnected = false;
        alignas(128) SHMQ SendQueue;
        alignas(128) SHMQ RecvQueue;
    }TChannel;
    TChannel* m_AllChannel;
    static volatile bool m_Stopped;
    std::thread* m_pInternalThread = nullptr;
    std::thread* m_pWorkThread = nullptr;
    T m_Msg;
    std::vector<int> m_CPU_List;
public:
    static SPSCQueue<T, Conf::ShmQueueSize * 4> m_SendQueue;
    static SPSCQueue<T, Conf::ShmQueueSize * 4> m_RecvQueue;
};

template <class T, class Conf> 
volatile bool SHMServer<T, Conf>::m_Stopped = false;
template <class T, class Conf> 
SPSCQueue<T, Conf::ShmQueueSize * 4> SHMServer<T, Conf>::m_SendQueue;
template <class T, class Conf> 
SPSCQueue<T, Conf::ShmQueueSize * 4> SHMServer<T, Conf>::m_RecvQueue;

} // namespace SHMIPC

#endif // SHMSERVER_HPP