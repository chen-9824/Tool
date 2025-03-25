cmake_minimum_required(VERSION 3.10.0)
project(push VERSION 0.1.0 LANGUAGES C CXX)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})

add_executable(OpenCvUvcPusher OpenCvUvcPusher.cpp FFmpegRTSPStreamer.cpp)
target_link_libraries(OpenCvUvcPusher ${OpenCV_LIBS} -lavcodec -lavformat -lavutil -lswresample -lswscale -lswscale -lavfilter)


add_executable(FFmpegUvcPusher FFmpegUvcPusher.cpp)
target_link_libraries(FFmpegUvcPusher  -lavcodec -lavformat -lavutil -lswresample -lswscale -lswscale -lavfilter -lavdevice)
