cmake_minimum_required(VERSION 3.10.0)
project(rknn VERSION 0.1.0 LANGUAGES C CXX)


# 指定目标架构
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_SYSTEM_PROCESSOR aarch64)

#set(Target_SYS "x86_64")
set(Target_SYS "aarch64")
#set(Target_SYS "ARM32")

if (Target_SYS STREQUAL "x86_64")
    message(STATUS "Building for x86_64")
elseif (Target_SYS STREQUAL "aarch64")
    message(STATUS "Building for aarch64")
    set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/spdlog/lib/cmake/spdlog/")
    find_package(spdlog REQUIRED)
elseif (Target_SYS STREQUAL "ARM32")
    message(STATUS "Building for ARM32")
    set(CMAKE_C_COMPILER /home/cfan/Desktop/RV1126_RV1109_SDK_2.1.0/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-gcc)
    set(CMAKE_CXX_COMPILER /home/cfan/Desktop/RV1126_RV1109_SDK_2.1.0/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-g++)
    set(CMAKE_SYSROOT /home/cfan/Desktop/RV1126_RV1109_SDK_2.1.0/buildroot/output/rockchip_rv1126_rv1109/host/arm-buildroot-linux-gnueabihf/sysroot/)
else()
    message(STATUS "Unknown architecture: ${CMAKE_SYSTEM_PROCESSOR}")
endif()

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
include_directories(${CMAKE_SOURCE_DIR}/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/inih/)
include_directories(${CMAKE_SOURCE_DIR}/inih/cpp)

set(SRC_DIR "${CMAKE_SOURCE_DIR}/rknn_yolov5/3rdparty")
file(GLOB SOURCES "${SRC_DIR}/*.cpp" "${SRC_DIR}/*.c" "${SRC_DIR}/*.cc")

set(LIB_DIR "${CMAKE_SOURCE_DIR}/rknn_yolov5/lib")
link_directories(${LIB_DIR})


add_executable(rknn main.cpp rknn.cpp  RkRga.cpp ${SOURCES})

target_link_libraries(rknn -lrknnrt -lrga -lturbojpeg  ${OpenCV_LIBS}) 

add_executable(rtsp_rknn rtsp_rknn.cpp rknn.cpp ${SOURCES} 
${CMAKE_SOURCE_DIR}/receiver/RTSPStream.cpp 
${CMAKE_SOURCE_DIR}/MyTcpClient/MyTcpClient.cpp
${CMAKE_SOURCE_DIR}/inih/ini.c
${CMAKE_SOURCE_DIR}/inih/cpp/INIReader.cpp)

target_include_directories(rtsp_rknn PRIVATE ${CMAKE_SOURCE_DIR}/MyTcpClient)

target_link_libraries(rtsp_rknn -lrknnrt -lrga -lturbojpeg pthread  
-lavcodec -lavformat -lavutil -lswresample -lswscale -lswscale -lavfilter 
 ${OpenCV_LIBS} spdlog::spdlog) 

install(TARGETS rtsp_rknn RUNTIME DESTINATION  ${CMAKE_SOURCE_DIR}/install/bin)
install(DIRECTORY rknn_yolov5/model/ DESTINATION ${CMAKE_SOURCE_DIR}/install/model)
install(DIRECTORY rknn_yolov5/lib/ DESTINATION ${CMAKE_SOURCE_DIR}/install/lib)
install(FILES  config.ini DESTINATION ${CMAKE_SOURCE_DIR}/install/)

