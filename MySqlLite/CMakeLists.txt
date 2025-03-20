cmake_minimum_required(VERSION 3.10.0)
project(MySqlTest VERSION 0.1.0 LANGUAGES C CXX)

add_executable(MySqlTest main.cpp ${CMAKE_CURRENT_LIST_DIR}/src/MySqlLite.cpp)

target_include_directories(MySqlTest PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/)

