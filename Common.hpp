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


enum EMsgType
{
    EMSG_TYPE_DATA = 0,
    EMSG_TYPE_LOGIN = 1,
    EMSG_TYPE_SERVER_ACK = 2,
    EMSG_TYPE_CLIENT_ACK = 3,
    EMSG_TYPE_HEARTBEAT = 10,
};

template <class T>
struct TChannelMsg
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
        fprintf(stderr, "shm_open %s error\n", filename);
        fflush(stderr);
        return nullptr;
    }
    if(ftruncate(fd, sizeof(T) * ChannelSize)) 
    {
        fprintf(stderr, "ftruncate %s error\n", filename);
        fflush(stderr);
        close(fd);
        return nullptr;
    }
    T* ret = (T*)mmap(0, sizeof(T) * ChannelSize, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    close(fd);
    if(ret == MAP_FAILED) 
    {
        fprintf(stderr, "mmap %s error\n", filename);
        fflush(stderr);
        return nullptr;
    }
    return ret;
}

template<class T>
void shm_munmap(void* addr, uint16_t ChannelSize) 
{
    munmap(addr, sizeof(T) * ChannelSize);
}

// 获取虚拟地址对应的物理地址
static uint64_t vaddr_to_paddr(void *vaddr) 
{
    int fd = open("/proc/self/pagemap", O_RDONLY);
    if (fd == -1) 
    {
        fprintf(stderr, "打开/proc/self/pagemap失败\n");
        fflush(stderr);
        return 0;
    }

    // 计算页大小和页内偏移
    uint64_t page_size = sysconf(_SC_PAGESIZE);
    uintptr_t virtual_addr = (uintptr_t)vaddr;
    uint64_t page_offset = virtual_addr / page_size;
    uint64_t entry_offset = page_offset * sizeof(uint64_t);

    // 定位到对应的页表条目
    if (lseek(fd, entry_offset, SEEK_SET) == -1) 
    {
        fprintf(stderr, "lseek失败\n");
        fflush(stderr);
        close(fd);
        return 0;
    }

    // 读取页表条目
    uint64_t entry;
    ssize_t bytes_read = read(fd, &entry, sizeof(entry));
    if (bytes_read != sizeof(entry)) 
    {
        fprintf(stderr, "读取页表条目失败\n");
        fflush(stderr);
        close(fd);
        return 0;
    }
    close(fd);

    // 检查页面是否存在
    if (!(entry & (1ULL << 63))) 
    {
        fprintf(stderr, "页面未驻留物理内存\n");
        fflush(stderr);
        return 0;
    }

    // 提取物理页帧号(PFN)
    uint64_t pfn = entry & ((1ULL << 55) - 1);
    return (pfn * page_size) + (virtual_addr % page_size);
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