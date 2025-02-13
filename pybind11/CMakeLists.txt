cmake_minimum_required(VERSION 3.14)
project(SHMServerBinding)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 17)


macro(Get_CommitID APP_COMMITID)   
    find_package(Git QUIET)     # 查找Git，QUIET静默方式不报错
    if(GIT_FOUND)
      execute_process(          # 执行一个子进程
        COMMAND ${GIT_EXECUTABLE} rev-parse --short HEAD # 命令
        OUTPUT_VARIABLE ${APP_COMMITID}        # 输出字符串存入变量
        OUTPUT_STRIP_TRAILING_WHITESPACE    # 删除字符串尾的换行符
        ERROR_QUIET                         # 对执行错误静默
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} # 执行路径
        )
    endif()
endmacro() 

set(APP_COMMITID "")
Get_CommitID(APP_COMMITID)
message(STATUS "AppCommitID is ${APP_COMMITID}")

add_definitions(-DAPP_COMMITID=\"${APP_COMMITID}\")


# 查找pybind11
find_package(pybind11 REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/../../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/../)
include_directories(${CMAKE_SOURCE_DIR})

# HP-Scoket
include_directories(${CMAKE_SOURCE_DIR}/../../XAPI/HP-Socket/5.8.2/include/)
link_directories(${CMAKE_SOURCE_DIR}/../../XAPI/HP-Socket/5.8.2/lib)

# 添加SHMServerBinding模块
pybind11_add_module(shm_server SHMServerBinding.cpp)
target_link_libraries(shm_server PRIVATE pybind11::module rt pthread)

# 添加SHMConnectionBinding模块
pybind11_add_module(shm_connection SHMConnectionBinding.cpp)
target_link_libraries(shm_connection PRIVATE pybind11::module rt pthread)

# 添加PackMessageBinding模块
pybind11_add_module(pack_message PackMessageBinding.cpp)
target_link_libraries(pack_message PRIVATE pybind11::module rt pthread)

# 添加SPSCQueueBinding模块
pybind11_add_module(spscqueue SPSCQueueBinding.cpp)
target_link_libraries(spscqueue PRIVATE pybind11::module rt pthread)

# 添加ClassBinding模块
# pybind11_add_module(Class ClassBinding.cpp)
# target_link_libraries(Class PRIVATE pybind11::module rt pthread)

# 添加HPPackClientBinding模块
pybind11_add_module(hp_pack_client HPPackClientBinding.cpp)
target_link_libraries(hp_pack_client PRIVATE pybind11::module rt hpsocket4c) 




