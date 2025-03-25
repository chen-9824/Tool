cmake_minimum_required(VERSION 3.15)
project(networkDemo)

set(CMAKE_CXX_STANDARD 14)

add_executable(networkDemo main.cpp)
add_executable(echo_mpserv echo_mpserv.c)
add_executable(echo_mpclient echo_mpclient.c)
add_executable(select select.c)
add_executable(echo_select_serv echo_select_serv.c)