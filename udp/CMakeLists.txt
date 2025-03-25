cmake_minimum_required(VERSION 3.15)
project(udp)

set(CMAKE_CXX_STANDARD 11)

add_executable(udp main.cpp client.cpp client.h server.cpp server.h)