cmake_minimum_required(VERSION 3.15)
project(radarUDP)

set(CMAKE_CXX_STANDARD 11)


add_executable(radarUDP main.cpp udp.cpp udp.h)

target_link_libraries(radarUDP pthread)