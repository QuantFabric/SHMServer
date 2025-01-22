cmake_minimum_required(VERSION 3.14)
project(SHMServerBinding)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/../)

#SET(CMAKE_BUILD_TYPE "Release")
#SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_BUILD_TYPE "RelWithDebInfo")
set(CMAKE_CXX_STANDARD 17)

# 查找pybind11
find_package(pybind11 REQUIRED)

# 包含目录
include_directories(${CMAKE_SOURCE_DIR}/../../Utils/)
include_directories(${CMAKE_SOURCE_DIR}/../)
include_directories(${CMAKE_SOURCE_DIR})

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




