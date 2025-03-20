cmake_minimum_required(VERSION 3.15)
project(test)

set(CMAKE_CXX_STANDARD 14)

#aux_source_directory(${CMAKE_CURRENT_LIST_DIR}/../ SRCS1)

include_directories(${CMAKE_SOURCE_DIR}/../spdlog/include)

add_executable(test main.cpp ${CMAKE_CURRENT_LIST_DIR}/src/MyTcpClient.cpp)

target_include_directories(test PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/)
target_link_libraries(test pthread)