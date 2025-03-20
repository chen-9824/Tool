cmake_minimum_required(VERSION 3.10.0)
project(AudioPlayer VERSION 0.1.0 LANGUAGES C CXX)

include_directories(${CMAKE_SOURCE_DIR}/../spdlog/include)

# 查找 SDL2 库
find_package(SDL2 REQUIRED)

# 将 SDL2 头文件和库链接到项目
include_directories(${SDL2_INCLUDE_DIRS})


add_executable(AudioPlayer main.cpp ${CMAKE_CURRENT_LIST_DIR}/src/AudioPlayer.cpp)

target_include_directories(AudioPlayer PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src/)

target_link_libraries(AudioPlayer ${SDL2_LIBRARIES} pthread)

