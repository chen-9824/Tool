cmake_minimum_required(VERSION 3.10.0)
project(rknn VERSION 0.1.0 LANGUAGES C CXX)


# 指定目标架构
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

# 指定交叉编译工具链
#set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
#set(CMAKE_CXX_COMPILER aarch64-linux-gnu-g++)

if(RGA_SAMPLES_UTILS_COMPILED)
    message(STATUS "RGA utils module has already been compiled.")
else()
    message(STATUS "RGA utils module not compiled yet, compiling now...")
    add_subdirectory(./rga/utils)   # 添加 utils 子模块进行编译
    SET(RGA_SAMPLES_UTILS_COMPILED "true" CACHE STRING "")
endif()

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/rknn_yolov5/include)
include_directories(${CMAKE_SOURCE_DIR}/rga/utils)
include_directories(${CMAKE_SOURCE_DIR}/receiver)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/rknn_yolov5/3rdparty")
file(GLOB SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c" "${SRC_DIR}/*.cc")

set(LIB_DIR "${CMAKE_SOURCE_DIR}/rknn_yolov5/lib")
link_directories(${LIB_DIR})

add_executable(rknn main.cpp rknn.cpp  RkRga.cpp ${SOURCES})

target_link_libraries(rknn -lrknnrt -lrga -lturbojpeg) 

add_executable(rtsp_rknn rtsp_rknn.cpp rknn.cpp ${SOURCES} ${CMAKE_SOURCE_DIR}/receiver/RTSPStream.cpp)

target_link_libraries(rtsp_rknn -lrknnrt -lrga -lturbojpeg pthread  
-lavcodec -lavformat -lavutil -lswresample -lswscale -lswscale -lavfilter 
 ${OpenCV_LIBS} ) 

