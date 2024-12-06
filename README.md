## SHMServer
### 简介
- SHMServer是一个基于共享内存实现进程间通信的服务端，提供发布订阅模式和CS模式的通信。
- 根据不同客户端划分通信信道，每个信道包含一个SPSC发送队列和SPSC接收队列，服务端的发送队列和接收队列分别对应一个客户端的接收队列和发送队列。
- SPSC队列底层数据缓冲区大小在编译时固定，因此如果队列满时消息入队会失败，导致消息丢失，需要在服务器性能做压力测试后选择不同的队列大小，确保在压力测试时也不会出现消息丢失。



### 测试
- build.sh：编译构建测试用例
- echoserver_perftest.sh：EchoServer性能测试脚本，压力测试模式下RTT延迟最大约6000ns，轻量负载下RTT延迟根据payload大小变化，100ns-600ns，需要每次编译构建前修改PackMessage的大小。
- pubserver_test.sh：PubServer测试脚本