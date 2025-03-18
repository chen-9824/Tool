cmake_minimum_required(VERSION 3.10.0)
project(RadarSerial VERSION 0.1.0 LANGUAGES C CXX)

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


include_directories(${CMAKE_SOURCE_DIR}/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(RadarSerial LDIR05.cpp ${CMAKE_SOURCE_DIR}/src/RadarSerial.cpp)
target_precompile_headers(RadarSerial PRIVATE pch.h)

if (Target_SYS STREQUAL "aarch64")
target_link_libraries(RadarSerial  -lpthread spdlog::spdlog)
else()
endif()

target_link_libraries(RadarSerial  -lpthread )

add_executable(HLK_LD2451 main.cpp ${CMAKE_SOURCE_DIR}/src/HLK_LD2451.cpp ${CMAKE_SOURCE_DIR}/src/RadarSerial.cpp)
target_precompile_headers(HLK_LD2451 PRIVATE pch.h)

if (Target_SYS STREQUAL "aarch64")
target_link_libraries(HLK_LD2451  -lpthread spdlog::spdlog)
else()
target_link_libraries(HLK_LD2451  -lpthread)
endif()
