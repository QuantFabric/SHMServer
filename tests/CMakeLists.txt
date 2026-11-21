cmake_minimum_required(VERSION 3.14)
project(tests)


#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 17)


# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/../../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/../)
include_directories(${CMAKE_SOURCE_DIR})

add_executable(EchoClient EchoClient.cpp)
target_link_libraries(EchoClient pthread rt)    

add_executable(EchoServer EchoServer.cpp)
target_link_libraries(EchoServer pthread rt)  

add_executable(SubClient SubClient.cpp)
target_link_libraries(SubClient pthread rt)  

add_executable(PubServer PubServer.cpp)
target_link_libraries(PubServer pthread rt)  

add_executable(SHMClient SHMClient.cpp)
target_link_libraries(SHMClient pthread rt) 

add_executable(SPSCQueueTest SPSCQueueTest.cpp)
target_link_libraries(SPSCQueueTest pthread rt)
target_compile_options(SPSCQueueTest PRIVATE -O2)

add_executable(SPSCQueueBenchmark SPSCQueueBenchmark.cpp)
target_link_libraries(SPSCQueueBenchmark pthread rt)
target_compile_options(SPSCQueueBenchmark PRIVATE -O3 -DNDEBUG)

add_executable(MPMCQueueTest MPMCQueueTest.cpp)
target_link_libraries(MPMCQueueTest pthread rt)
target_compile_options(MPMCQueueTest PRIVATE -O2)

add_executable(MPMCQueueBenchmark MPMCQueueBenchmark.cpp)
target_link_libraries(MPMCQueueBenchmark pthread rt)
target_compile_options(MPMCQueueBenchmark PRIVATE -O3 -DNDEBUG)

add_executable(SHMIPCIntegrationTest SHMIPCIntegrationTest.cpp)
target_link_libraries(SHMIPCIntegrationTest pthread rt)
target_compile_options(SHMIPCIntegrationTest PRIVATE -O2)

add_executable(SHMIPerformanceBenchmark SHMIPerformanceBenchmark.cpp)
target_link_libraries(SHMIPerformanceBenchmark pthread rt)
target_compile_options(SHMIPerformanceBenchmark PRIVATE -O3 -DNDEBUG)

