cmake_minimum_required(VERSION 3.10.0)
project(push VERSION 0.1.0 LANGUAGES C CXX)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(push main.cpp FFmpegRTSPStreamer.cpp)


target_link_libraries(push ${OpenCV_LIBS} -lavcodec -lavformat -lavutil -lswresample -lswscale -lswscale -lavfilter)
