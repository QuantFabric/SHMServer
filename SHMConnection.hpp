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
    }

    virtual ~SHMConnection()
    {
        Release();
    }

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
                        ret = true;
                        fprintf(stdout, "SHMConnection Register new Channel:%d success, ClientName: %s\n", i, m_ClientName);
                        break;
                    }
                    else if(strncmp(m_AllChannel[i].ChannelName, m_ClientName, sizeof(m_AllChannel[i].ChannelName)) == 0) 
                    {
                        m_Channel = &m_AllChannel[i];
                        m_Channel->ChannelID = i;
                        m_Channel->SendQueue.Reset();
                        m_Channel->RecvQueue.Reset();
                        ret = true;
                        fprintf(stdout, "SHMConnection Register exist Channel:%d success, ClientName: %s\n", i, m_ClientName);
                        break;
                    }
                }
                fprintf(stdout, "SHMConnection Init done %.2f MB\n", sizeof(TChannel) * Conf::ChannelSize / 1024.0 / 1024.0);
            }
        }
        return ret;
    }

    bool Push(const T& msg) 
    {
        return m_Channel->SendQueue.Push(msg);
    }

    bool Pop(T& msg) 
    {
        return m_Channel->RecvQueue.Pop(msg);
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

    int32_t ChannelID()
    {
        int32_t ret = -1;
        if(m_Channel) 
        {
            ret = m_Channel->ChannelID;
        }
        return ret;
    }

    void Reset() 
    {
        if(m_Channel) 
        {
            m_Channel->SendQueue.Reset();
            m_Channel->RecvQueue.Reset();
        }
    }

private:
    char m_ClientName[Conf::NameSize];

    using SHMQ = SPSCQueue<T, Conf::ShmQueueSize>;
    typedef struct
    {
        char ChannelName[Conf::NameSize];
        int32_t ChannelID = -1;
        bool IsConnected = false;
        alignas(128) SHMQ RecvQueue;
        alignas(128) SHMQ SendQueue;
    }TChannel;
    TChannel* m_Channel;
    TChannel* m_AllChannel;

};
} // namespace SHMIPC

#endif // SHMCONNECTION_HPP