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




