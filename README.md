## SHMServer
### 简介
- SHMServer是一个基于共享内存实现进程间通信的服务端，提供发布订阅模式和CS模式的通信。
- 根据不同客户端划分通信信道，每个信道包含一个SPSC发送队列和SPSC接收队列，服务端的发送队列和接收队列分别对应一个客户端的接收队列和发送队列。
- SPSC队列底层数据缓冲区大小在编译时固定，因此如果队列满时消息入队会失败，导致消息丢失，需要在服务器性能做压力测试后选择不同的队列大小，确保在压力测试时也不会出现消息丢失。
- SHMConnection为了适配服务端，也需要定义Publish发布订阅模式和CS模式，主要在于发布订阅模式对内存通道数量的需求要远大于CS模式。
- SHMConnection使用非性能模式时，Pop前需要执行HandleMsg回调函数，Push后需要执行HandleMsg回调函数，同时在程序的主循环内执行HandleMsg回调函数处理HEARBEAT消息。
- SHMServer使用非性能模式时，从客户端接收数据前需要执行PollMsg回调函数，发送数据到客户端后需要执行PollMsg回调函数，同时在程序的主循环内执行PollMsg回调函数处理HEARBEAT消息。

### SPSCQueue低延迟优化
- SPSCQueue使用固定容量N的环形缓冲区，保留一个空槽用于区分满载和空载，因此可写入容量为`N-1`。
- 生产者和消费者分别缓存对端索引。成功执行`Push`或`Pop`时不再读取对端计数器，仅在队列疑似满或空时使用acquire语义重新加载。
- 缓存索引复用原有两个128字节计数行中的填充空间，`m_Head`、`m_Tail`的位置以及队列对象大小保持不变。生产者和消费者仍分别拥有独立的计数缓存行，避免新增伪共享。
- 数据发布使用release语义，对端在满/空慢路径使用acquire语义，保证槽位内容在发布可见前完成写入。
- `Reset()`不再对整个消息数组执行`memset`，只重置头、尾及两个缓存索引。默认发布模式下，每个通道包含两个8 MiB队列，原有清零逻辑会在初始化时触碰约4 GiB共享内存页面；优化后消息页面按实际访问产生。
- 队列要求元素类型满足trivially copyable，并检查`std::atomic<uint32_t>`始终无锁。`Push/Pop/Reset`的函数签名、N-1容量和满/空返回`false`的语义保持不变。
- `Reset()`只能在线程和进程都停止访问队列时调用，不是并发安全的重置接口。
- `SPSCQueue`对象大小和计数器偏移保持兼容，但新增的缓存索引需要生产端和消费端使用同一版本构建结果。升级时应协调重启客户端和服务端，并重新创建共享内存映射。

#### 性能对比
- 测试环境：Intel Xeon E5-2673 v4，Linux x86_64，`-O3`，生产者绑定CPU 0、消费者绑定同Socket的不同物理核CPU 2。
- 测试方法：旧实现与优化实现交替运行，预热后执行7轮A/B，取中位数。同进程测试发送500,000条消息，跨进程测试发送100,000条消息。
- 64字节载荷、队列大小1024：

| 场景 | 优化前 | 优化后 | 吞吐变化 |
|---|---:|---:|---:|
| 同进程1P1C | 9.32 Mmsg/s | 15.71 Mmsg/s | +68.6% |
| 跨进程1P1C | 8.27 Mmsg/s | 16.45 Mmsg/s | +98.9% |

- 1024字节载荷、队列大小1024：

| 场景 | 优化前 | 优化后 | 吞吐变化 |
|---|---:|---:|---:|
| 同进程1P1C | 1.62 Mmsg/s | 1.70 Mmsg/s | +4.9% |
| 跨进程1P1C | 1.65 Mmsg/s | 1.92 Mmsg/s | +16.8% |

- `Reset()`测试：

| 队列大小 | 优化前 | 优化后 |
|---|---:|---:|
| 64 KiB | 约3.16 us | 约1.6 ns |
| 8 MiB | 约567 us | 约1.6 ns |

- 优化后的`Reset()`耗时不再随消息数组大小线性增长。小消息场景主要受益于减少对端缓存行读取，1 KiB消息场景的数据复制占比较高，因此吞吐提升较小。
- 当前测试运行在虚拟化环境中，线程调度和p99等高百分位延迟存在明显波动，上述吞吐用于同机A/B比较，不应直接作为生产环境SLA。

#### 构建与运行
```bash
cmake -S SHMServer/tests -B SHMServer/tests/build
cmake --build SHMServer/tests/build --target SPSCQueueTest SPSCQueueBenchmark -j

./SHMServer/tests/build/SPSCQueueTest
./SHMServer/tests/build/SPSCQueueBenchmark --runs 7
```

- `SPSCQueueTest`覆盖容量边界、长环绕、Reset、不同队列隔离、生产/消费速率失衡、线程并发和fork跨进程场景；`--skip-fork`可跳过跨进程测试。
- `SPSCQueueBenchmark`内置旧实现作为A/B基线，测试64字节和1024字节载荷，同进程与跨进程吞吐、p50/p99以及`Reset()`耗时。

### MPMCQueue低延迟优化
- MPMCQueue继续使用全局头尾CAS预约槽位和每槽状态发布数据，保持N容量、FIFO顺序、满/空返回`false`以及`Capacity()`、`Size()`的原有语义。
- 每个`QueueNode`改为64字节对齐的数据存储加单个`sequence`原子量，并使用`new QueueNode[N]`/`delete[]`分配。原实现通过普通`char[]`模拟64字节对齐，未满足实际的over-aligned分配要求，压力测试中可触发`SIGSEGV`。
- N大于1时，节点使用标准连续ticket：空闲状态为`index`，发布状态为`index + 1`，出队后下一轮空闲状态为`head + N`。N=1时使用双倍ticket，避免同一槽位的空闲和发布状态发生编码冲突。
- 生产者使用acquire检查槽位是否可复用，写入完成后使用release发布；消费者使用acquire确认数据已发布，复制完成后使用release归还槽位。
- 删除运行时`m_QueueMask`成员，统一使用编译期`N - 1`，减少每次操作的成员读取和对象缓存行占用。
- 槽内元素通过`aligned_storage_t`、placement new、`std::launder`管理。元素类型必须满足trivially copyable、trivially copy-assignable和trivially destructible，避免槽位预留后复制抛异常导致队列永久阻塞。
- 全局`m_Tail`和`m_Head`仍分别使用128字节对齐。多生产者/消费者共享计数器的CAS竞争仍是主要延迟来源，本次没有引入批处理、分布式计数器或新的回收机制。
- `MPMCQueue`对象大小由512字节降为384字节，函数签名不变，但调用方需要重新编译，不应跨版本混用对象ABI。
- 头槽位生产者尚未发布时，消费者仍可能暂时返回空，不额外承诺强lock-free前进保证。

#### 性能对比
- 测试环境：Intel Xeon E5-2673 v4，Linux x86_64，`-O3`，线程绑定CPU 0、2、4、6、8、10、12、14。
- 测试方法：A侧为修复节点对齐和内存序之后的原双状态算法，B侧为单序列号算法；每个拓扑共发送500,000条消息，预热后交替运行7轮并取中位数。队列大小1024。
- 64字节载荷：

| 拓扑 | 双状态基线 | 单序列号版本 | 吞吐变化 |
|---|---:|---:|---:|
| 1P1C | 12.27 Mmsg/s | 11.59 Mmsg/s | -5.5% |
| 2P2C | 6.33 Mmsg/s | 7.03 Mmsg/s | +11.1% |
| 4P4C | 4.23 Mmsg/s | 5.92 Mmsg/s | +40.0% |

- 1024字节载荷：

| 拓扑 | 双状态基线 | 单序列号版本 | 吞吐变化 |
|---|---:|---:|---:|
| 1P1C | 1.84 Mmsg/s | 1.84 Mmsg/s | -0.2% |
| 2P2C | 2.84 Mmsg/s | 2.84 Mmsg/s | -0.1% |
| 4P4C | 3.76 Mmsg/s | 3.78 Mmsg/s | +0.3% |

- 单序列号版本在高竞争场景通常持平或改善，但低竞争64B/1P1C仍约有5%回退。该拓扑没有MPMC需求时应优先使用SPSCQueue。
- 当前测试运行在虚拟化环境中，线程调度会使吞吐和p99出现较大波动，尤其是4P4C数据；上述结果仅用于同机A/B比较，不应直接作为生产环境SLA。

#### 构建与运行
```bash
cmake -S SHMServer/tests -B SHMServer/tests/build
cmake --build SHMServer/tests/build --target MPMCQueueTest MPMCQueueBenchmark -j

./SHMServer/tests/build/MPMCQueueTest
./SHMServer/tests/build/MPMCQueueBenchmark --runs 7
```

- `MPMCQueueTest`覆盖N容量边界、N=1、长环绕、队列隔离、1P1C/2P2C/4P4C并发、生产/消费速率失衡，以及无丢失、无重复和完整载荷校验。
- `--quick`使用较小压力执行同一套场景，适合Valgrind等较慢工具：
    ```bash
    valgrind --tool=memcheck --error-exitcode=99 --leak-check=full \
        ./SHMServer/tests/build/MPMCQueueTest --quick
    ```
- `MPMCQueueBenchmark`内置对齐后的原双状态算法作为A/B基线，测试64字节和1024字节载荷、1P1C/2P2C/4P4C吞吐以及p50/p99。

### SHMServer与SHMConnection优化

#### 优化内容

- `LOGIN/SERVER_ACK/CLIENT_ACK/HEARTBEAT`从数据队列迁移到独立控制队列，数据通道只承载`TDataMsg<T>`。
- 控制消息使用24字节`TControlMsg`；控制队列容量为64，可用容量为63。
- 每个通道拥有独立`ControlGeneration`。客户端控制消息成功入队后递增本通道代际，服务端只扫描代际变化的通道控制队列。
- `SHMConnection`不再持有本地数据队列。`Start()`成功后，`Push`直接写通道数据发送队列，`Pop`直接读通道数据接收队列，控制工作线程不再搬运业务数据。
- 未`Start`或连接失败时，客户端`Push/Pop`立即返回`false`；不提供启动前数据缓冲。
- 客户端删除`m_DirectDataPath`及其分支，每次连接后的`Push/Pop`不再读取本地数据队列或检查直连开关。
- `SHMConnection::Start`只接受服务端名称，不再接收CPU编号；心跳线程由操作系统调度，不执行CPU绑定。
- 服务端数据聚合顺序、发布扇出和每客户端独立满队列丢弃语义保持不变。
- `m_Stopped`、`m_IsConnected`和工作线程时间戳使用原子量；客户端握手缓冲改为调用级局部对象，`RDTSC()`使用寄存器局部变量。
- `fflush`、逐消息`fprintf`、`usleep(1000)`和心跳TSC检查频率保持不变。

#### 共享内存与兼容性

| 项目 | 优化前 | 优化后 |
|---|---:|---:|
| 控制消息尺寸 | 1024字节 | 24字节 |
| 每通道共享内存 | 约16.00 MiB | 约16.00 MiB + 3.5 KiB |
| 256通道发布模式共享区 | 约4096.06 MiB | 约4097.03 MiB |
| 8通道CS模式共享区 | 约128.00 MiB | 约128.03 MiB |

- 共享区域使用独立的`alignas(128) std::atomic<uint64_t> Version`，当前十六进制版本为`0xFF01`；服务端初始化时写0，全部通道完成后发布`0xFF01`。每通道另有独立128字节对齐控制代际。
- 客户端只映射已存在且尺寸完全匹配的共享对象，避免旧服务端或旧客户端误连。
- 服务端和客户端必须使用同一版本重新构建、停止旧进程并重新创建共享内存对象。

#### 端到端性能对比

- 测试环境：Intel Xeon E5-2673 v4，Linux x86_64，`-O3`。
- CPU绑定：SHMServer线程和测试应用/发布消费线程按测试配置绑定；SHMConnection心跳线程不绑定CPU。
- 对比方法：基线二进制与候选二进制按平衡顺序交替运行8轮，每侧4轮；表中为轮次中位数。
- 数据量：同进程Publish A/B使用1024字节消息和3000条发布消息；扩展矩阵使用每客户端固定消息数。
- 测量保留生产日志、`fflush`和`usleep(1000)`，标准输出重定向到`/dev/null`。
- 8轮A/B用于评估客户端数据直连，测量时代际控制扫描仍启用；下方完整矩阵为无代际基线，随后给出`0xFF01`每通道代际版本的关键场景对比。

CS Echo结果：

| 场景 | 优化前p50 | 优化后p50 | p50变化 | 优化前吞吐 | 优化后吞吐 |
|---|---:|---:|---:|---:|---:|
| 128B 1客户端 | 2.829 us | 2.053 us | -27.4% | 0.31 Mmsg/s | 0.40 Mmsg/s |
| 128B 2客户端 | 3.133 us | 2.064 us | -34.1% | 0.27 Mmsg/s | 0.405 Mmsg/s |
| 1024B 1客户端 | 6.797 us | 2.369 us | -65.1% | 0.12 Mmsg/s | 0.19 Mmsg/s |
| 1024B 2客户端 | 7.256 us | 2.391 us | -67.0% | 0.115 Mmsg/s | 0.195 Mmsg/s |

- 128B场景p99约降低29%～31%。
- 1024B双客户端p99约降低18%；1024B单客户端p99中位数约回退8%，表明直连路径仍受调用线程日志和调度抖动影响。
- 客户端直连去掉了本地数据队列一进一出和相应载荷复制，是数据RTT下降的主要来源。

Publish结果：

| 场景 | 优化前p50 | 优化后p50 | 优化前p99 | 优化后p99 | 优化前吞吐 | 优化后吞吐 |
|---|---:|---:|---:|---:|---:|---:|
| 1客户端 | 0.910 ms | 0.794 ms | 1.857 ms | 1.887 ms | 0.80 Mdel/s | 0.755 Mdel/s |
| 5客户端 | 7.701 ms | 7.407 ms | 15.216 ms | 12.305 ms | 0.705 Mdel/s | 0.985 Mdel/s |

- 发布模式继续保持服务端扇出结构和逐客户端独立队列。
- 日志、`fflush`和握手`sleep`仍会显著稀释低延迟收益；本次没有优化这些路径。
- 虚拟化环境存在调度抖动，结果用于同机基线/候选比较，不应直接作为生产SLA。

#### 扩展客户端矩阵

以下完整矩阵在此前的无ControlGeneration基线上执行一次完整运行，保留作为布局和客户端扩展基准。CS模式将基准配置的通道数扩展到16；Publish模式的大客户端数场景使用独立客户端进程，避免单个进程重复映射完整发布共享区。表中Mdel/s表示每秒客户端投递数。

CS Echo，5/10客户端：

| 载荷 | 5客户端p50 | 5客户端p99 | 5客户端吞吐 | 10客户端p50 | 10客户端p99 | 10客户端吞吐 |
|---|---:|---:|---:|---:|---:|---:|
| 128B | 2.939 us | 10.655 us | 0.15 Mmsg/s | 2.208 us | 10.294 us | 0.16 Mmsg/s |
| 256B | 2.948 us | 10.243 us | 0.13 Mmsg/s | 2.322 us | 10.077 us | 0.15 Mmsg/s |
| 512B | 2.786 us | 14.072 us | 0.11 Mmsg/s | 2.810 us | 13.231 us | 0.12 Mmsg/s |
| 1024B | 3.307 us | 16.260 us | 0.09 Mmsg/s | 3.354 us | 15.966 us | 0.08 Mmsg/s |

Publish，32/64/128/256客户端：

| 载荷 | 客户端数 | p50 | p99 | 投递吞吐 |
|---|---:|---:|---:|---:|
| 128B | 32 | 20.239 ms | 31.905 ms | 0.18 Mdel/s |
| 128B | 64 | 30.250 ms | 67.228 ms | 0.13 Mdel/s |
| 128B | 128 | 62.952 ms | 138.177 ms | 0.12 Mdel/s |
| 128B | 256 | 175.479 ms | 327.098 ms | 0.11 Mdel/s |
| 256B | 32 | 25.342 ms | 39.198 ms | 0.12 Mdel/s |
| 256B | 64 | 27.721 ms | 72.202 ms | 0.11 Mdel/s |
| 256B | 128 | 72.673 ms | 139.159 ms | 0.14 Mdel/s |
| 256B | 256 | 186.618 ms | 323.718 ms | 0.09 Mdel/s |
| 512B | 32 | 32.642 ms | 44.403 ms | 0.11 Mdel/s |
| 512B | 64 | 53.658 ms | 81.504 ms | 0.12 Mdel/s |
| 512B | 128 | 80.051 ms | 169.179 ms | 0.12 Mdel/s |
| 512B | 256 | 248.411 ms | 418.395 ms | 0.10 Mdel/s |
| 1024B | 32 | 26.945 ms | 44.213 ms | 0.17 Mdel/s |
| 1024B | 64 | 62.348 ms | 98.888 ms | 0.12 Mdel/s |
| 1024B | 128 | 128.250 ms | 207.355 ms | 0.11 Mdel/s |
| 1024B | 256 | 228.889 ms | 405.085 ms | 0.11 Mdel/s |

- Publish延迟随客户端数近似增长，主要成本仍是服务端向每个客户端复制完整消息。
- 256客户端场景同时受到CPU超订、调度、进程启动和握手成本影响；不同单次运行之间可能存在吞吐离群值，不应解释为稳定扩展趋势。
- CS模式在10客户端内保持较低p50，1024B时p99和吞吐开始受日志与数据复制成本影响。

每通道ControlGeneration关键场景对比：

- 对比版本：无代际基线 vs `0xFF01`每通道代际版本。
- 相邻运行顺序：无代际基线后运行每通道代际候选。
- 场景：CS 128B/1024B的5/10客户端，Publish 128B/1024B的32/256客户端。

| 场景 | 无代际p50 | 每通道代际p50 | 变化 |
|---|---:|---:|---:|
| CS 128B 5C | 1.891 us | 2.253 us | +19.1% |
| CS 128B 10C | 2.032 us | 1.490 us | -26.7% |
| CS 1024B 5C | 3.011 us | 2.476 us | -17.8% |
| CS 1024B 10C | 2.774 us | 2.492 us | -10.2% |
| Publish 128B 32C | 40.174 ms | 36.372 ms | -9.5% |
| Publish 128B 256C | 393.051 ms | 290.057 ms | -26.2% |
| Publish 1024B 32C | 22.828 ms | 34.891 ms | +52.8% |
| Publish 1024B 256C | 226.058 ms | 454.982 ms | +101.3% |

- CS测试表明代际筛选可减少未变化通道的控制队列`Pop`。
- Publish高客户端数仍受进程启动、CPU超订和调度主导，单次相邻结果不足以证明稳定收益或回退。
- 服务端仍需每轮读取各通道代际并执行心跳超时检查，因此不会消除全部通道遍历。
- 每通道代际消除了全局代际缓存行竞争，并避免一个客户端事件触发所有通道控制队列扫描。

#### 客户端本地数据队列移除对比

- 基线：保留两个本地数据SPSC和`m_DirectDataPath`，连接后走直连。
- 候选：删除两个本地SPSC、`m_DataMsg`和`m_DirectDataPath`，连接后始终直接访问共享通道。
- 使用相同关键矩阵基准，先运行基线再运行候选。

| 场景 | 基线p50 | 候选p50 | p50变化 | 基线吞吐 | 候选吞吐 |
|---|---:|---:|---:|---:|---:|
| CS 128B 5C | 2.202 us | 1.362 us | -38.1% | 0.20 Mmsg/s | 0.26 Mmsg/s |
| CS 128B 10C | 1.648 us | 1.531 us | -7.1% | 0.24 Mmsg/s | 0.26 Mmsg/s |
| CS 1024B 5C | 2.339 us | 2.465 us | +5.4% | 0.10 Mmsg/s | 0.10 Mmsg/s |
| CS 1024B 10C | 2.388 us | 2.407 us | +0.8% | 0.10 Mmsg/s | 0.10 Mmsg/s |
| Publish 128B 32C | 13.367 ms | 17.643 ms | +32.0% | 0.17 Mdel/s | 0.13 Mdel/s |
| Publish 128B 256C | 231.350 ms | 188.669 ms | -18.4% | 0.12 Mdel/s | 0.11 Mdel/s |
| Publish 1024B 32C | 23.555 ms | 57.032 ms | +142.1% | 0.12 Mdel/s | 0.09 Mdel/s |
| Publish 1024B 256C | 439.291 ms | 232.231 ms | -47.1% | 0.09 Mdel/s | 0.10 Mdel/s |

- 连接后的候选版本原本已经使用共享数据队列，因此稳态延迟的预期变化很小。表中大幅变化主要受CPU调度和进程启动影响，不能解释为稳定延迟趋势。
- 明确收益来自资源消除：`PackMessage`客户端构造后的虚拟内存增量由约15.5 MiB降为0，对应两个本地8 MiB SPSC队列。
- 删除每次`Push/Pop`的`m_DirectDataPath`原子读取和分支，但该成本远小于IPC复制和日志成本。
- 未连接时不再接受数据入队，`Push/Pop`直接返回`false`。

#### 测试与运行

```bash
cmake -S SHMServer/tests -B SHMServer/tests/build
cmake --build SHMServer/tests/build \
  --target SHMIPCIntegrationTest SHMIPerformanceBenchmark -j

./SHMServer/tests/build/SHMIPCIntegrationTest
./SHMServer/tests/build/SHMIPerformanceBenchmark > /dev/null
```

- `SHMIPCIntegrationTest`覆盖双客户端登录/ACK、注入心跳处理、未连接`Push/Pop`拒绝、重连、两个独立共享通道的数据往返和停止流程。
- `SHMIPerformanceBenchmark`覆盖128B/256B/512B/1024B数据；CS模式测试1/2/5/10客户端，Publish模式测试1/5客户端以及32/64/128/256独立客户端进程。

### Python扩展

当前 Python 绑定位于 `SHMServer/python`，使用 SWIG 生成，模块名和主要
Python 接口与原有 pybind11 绑定保持一致。构建主机需要 Python 3.9+
开发头文件、CMake、C++17 编译器和 SWIG 4.0+，运行时需要 NumPy。
CMake 会自动调用 SWIG，将包装代码生成到 `build/generated`，无需提前
生成或提交 `*_wrap.cxx`。

```bash
cd SHMServer/python
cmake -S . -B build -DCMAKE_BUILD_TYPE=RelWithDebInfo
cmake --build build -j
ctest --test-dir build --output-on-failure
```

运行时设置：

```bash
export PYTHONPATH=/path/to/QuantFabric/SHMServer/python/build:/path/to/QuantFabric/SHMServer/python/build/generated:$PYTHONPATH
```

生成模块包括 `pack_message`、`shm_server`、`shm_connection` 和
`spscqueue`，扩展文件名包含 Python ABI 后缀。也可通过
`scikit-build-core` 构建并安装 wheel：

```bash
cd SHMServer/python
python3 -m pip wheel . --no-deps --wheel-dir dist
python3 -m pip install --force-reinstall dist/shmserver-0.1.0-*.whl
```

详细依赖、构建期 SWIG 生成、wheel/sdist 打包和使用示例见
`SHMServer/python/README.md`。

原有 `SHMServer/pybind11` 目录继续保留用于对照和回退，不参与新的 SWIG
构建。



### 测试
- build.sh：编译构建测试用例
- perftest.sh：性能测试脚本，RTT延迟根据payload大小变化，100ns-600ns，需要每次编译构建前修改PackMessage的大小。
- echoserver_benchmark.sh：EchoServer压力测试脚本，RTT延迟最大约6000ns。
- pubserver_test.sh：PubServer测试脚本
- SHMClient：C++客户端测试用例，从共享内存通道接收数据。
    ```bash
    SHMClient <ClientName> <ServerName>
    ```
