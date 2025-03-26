cmake_minimum_required(VERSION 3.10.0)
project(rknn VERSION 0.1.0 LANGUAGES C CXX)

# 指定目标架构
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 指定交叉编译工具链
set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)



include_directories(${CMAKE_SOURCE_DIR}/rknn_yolov5/include)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/rknn_yolov5/3rdparty")
file(GLOB SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c" "${SRC_DIR}/*.cc")

set(LIB_DIR "${CMAKE_SOURCE_DIR}/rknn_yolov5/lib")
link_directories(${LIB_DIR})

add_executable(rknn main.cpp rknn.cpp ${SOURCES})

target_link_libraries(rknn -lrknnrt -lrga -lturbojpeg)

