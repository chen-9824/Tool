cmake_minimum_required(VERSION 3.10.0)
project(RadarSerial VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_PREFIX_PATH "${CMAKE_SOURCE_DIR}/spdlog/lib/cmake/spdlog/")
find_package(spdlog REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/spdlog/include)
include_directories(${CMAKE_SOURCE_DIR}/src)

add_executable(RadarSerial LDIR05.cpp ${CMAKE_SOURCE_DIR}/src/RadarSerial.cpp)
target_precompile_headers(RadarSerial PRIVATE pch.h)
target_link_libraries(RadarSerial  -lpthread spdlog::spdlog)

add_executable(HLK_LD2451 main.cpp ${CMAKE_SOURCE_DIR}/src/HLK_LD2451.cpp ${CMAKE_SOURCE_DIR}/src/RadarSerial.cpp)
target_precompile_headers(HLK_LD2451 PRIVATE pch.h)
target_link_libraries(HLK_LD2451  -lpthread spdlog::spdlog)
