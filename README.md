## SHMServer
### 简介
- SHMServer是一个基于共享内存实现进程间通信的服务端，提供发布订阅模式和CS模式的通信。
- 根据不同客户端划分通信信道，每个信道包含一个SPSC发送队列和SPSC接收队列，服务端的发送队列和接收队列分别对应一个客户端的接收队列和发送队列。
- SPSC队列底层数据缓冲区大小在编译时固定，因此如果队列满时消息入队会失败，导致消息丢失，需要在服务器性能做压力测试后选择不同的队列大小，确保在压力测试时也不会出现消息丢失。
- **SHMServer的SHMserver和SHMConnection组件使用时需要修改栈大小为16MB，否则会出现段错误**。
    ```bash
    ulimit -s 16384
    ```
- SHMConnection使用非性能模式时，Pop前需要执行HandleMsg回调函数，Push后需要执行HandleMsg回调函数，同时在程序的主循环内执行HandleMsg回调函数处理HEARBEAT消息。
- SHMserver使用非性能模式时，从客户端接收数据前需要执行PollMsg回调函数，发送数据到客户端后需要执行PollMsg回调函数，同时在程序的主循环内执行PollMsg回调函数处理HEARBEAT消息。

### Python扩展
- 安装依赖：
    ```
    yum install python3-devel
    ```
- pybind11文件夹内py测试用例必须使用python3执行。
    - shm_server_test.py：SHMServer测试用例。
    - shm_connection_test.py：SHMConnection测试用例，连接SHMServer服务器。
    - market_client_test.py：行情客户端测试用例，可以连接XMarketCenter行情服务，从共享内存通道接收行情数据。
    - quant_server_test.py：QuantServer测试用例，QuantServer服务器，从共享内存通道接收行情数据，通过共享内存通道发送订单信息。
    - spscqueue_test.py: SPSC队列测试用例。



### 测试
- build.sh：编译构建测试用例
- perftest.sh：性能测试脚本，RTT延迟根据payload大小变化，100ns-600ns，需要每次编译构建前修改PackMessage的大小。
- echoserver_benchmark.sh：EchoServer压力测试脚本，RTT延迟最大约6000ns。
- pubserver_test.sh：PubServer测试脚本
- SHMClient：C++客户端测试用例，从共享内存通道接收数据。
    ```bash
    SHMClient <ClientName> <ServerName>
    ```