cmake_minimum_required(VERSION 3.10.0)
project(RKMPP VERSION 0.1.0 LANGUAGES C CXX)

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

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

include_directories(${CMAKE_SOURCE_DIR}/receiver)
include_directories(${CMAKE_SOURCE_DIR}/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/rkmpp/include/rockchip/)

# Operation System Abstract Layer (OSAL) include
include_directories(rkmpp/osal/inc)
# OSAL is needed on all platform, do not need option
add_subdirectory(rkmpp/osal)

#include_directories(rkmpp/utils)
#file(GLOB SRC_FILES "${CMAKE_SOURCE_DIR}/rkmpp/utils/*.c")



add_executable(RKMPP main.cpp RkMpp.cpp ${CMAKE_SOURCE_DIR}/receiver/RTSPStream.cpp ${CMAKE_SOURCE_DIR}/receiver/MppRtspRecv.cpp ${SRC_FILES})

#target_include_directories(RKMPP PRIVATE rkmpp/include/rockchip/)
target_link_directories(RKMPP PRIVATE rkmpp/lib)

target_link_libraries(RKMPP rockchip_mpp spdlog::spdlog 
-lavcodec -lavformat -lavutil -lswresample -lswscale -lswscale -lavfilter 
 ${OpenCV_LIBS})

set_target_properties(RKMPP PROPERTIES
    BUILD_RPATH "rkmpp/lib"
)