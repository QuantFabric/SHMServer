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
};

template <class T>
struct ChannelMsg
{
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

} // namespace SHMIPC

#endif // COMMON_HPP