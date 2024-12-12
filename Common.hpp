#ifndef COMMON_HPP
#define COMMON_HPP

#include <sys/mman.h>
#include <sys/stat.h>        
#include <fcntl.h>           
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <thread>
#include <sched.h>

namespace SHMIPC{

struct CommonConf
{
    static const uint32_t NameSize = 32;
    static const uint32_t ShmQueueSize = 1024 * 8; 
    static const uint32_t ChannelSize = 8;
    static const uint64_t Heartbeat_Interval = 5 * 60 * 1000 * 1000 * 1000UL;
};


enum MsgType
{
    MSG_TYPE_DATA = 0,
    MSG_TYPE_LOGIN = 1,
    MSG_TYPE_SERVER_ACK = 2,
    MSG_TYPE_CLIENT_ACK = 3,
    MSG_TYPE_HEARTBEAT = 10,
};

template <class T>
struct ChannelMsg
{
    uint32_t MsgType;
    uint64_t MsgID;
    uint64_t TimeStamp;
    int32_t ChannelID;
    T Data;
};


template<class T>
T* shm_mmap(const char* filename, uint16_t ChannelSize) 
{
    // 创建到/dev/shm目录下
    int fd = shm_open(filename, O_CREAT | O_RDWR, 0666);
    if(fd == -1) 
    {
        printf("shm_open %s error\n", filename);
        return nullptr;
    }
    if(ftruncate(fd, sizeof(T) * ChannelSize)) 
    {
        printf("ftruncate %s error\n", filename);
        close(fd);
        return nullptr;
    }
    T* ret = (T*)mmap(0, sizeof(T) * ChannelSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if(ret == MAP_FAILED) 
    {
        printf("mmap %s error\n", filename);
        return nullptr;
    }
    return ret;
}

template<class T>
void shm_munmap(void* addr, uint16_t ChannelSize) 
{
    munmap(addr, sizeof(T) * ChannelSize);
}

static bool ThreadBind(pthread_t thread, int cpuid)
{
    cpu_set_t mask;
    CPU_ZERO(&mask);
    CPU_SET(cpuid, &mask);
    return pthread_setaffinity_np(thread, sizeof(mask), &mask) == 0;
}

static inline uint64_t RDTSC() 
{
    static uint32_t hi, lo;
    __asm__ __volatile__ ("rdtsc" : "=a"(lo), "=d"(hi));
    return ( (uint64_t)lo) | (((uint64_t)hi) << 32);
}

    static inline uint64_t RDTSCP() 
    {
        unsigned int d = 0;
        return __builtin_ia32_rdtscp(&d);
    }

} // namespace SHMIPC

#endif // COMMON_HPP