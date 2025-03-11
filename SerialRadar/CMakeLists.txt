cmake_minimum_required(VERSION 3.10.0)
project(RadarSerial VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/spdlog/lib/cmake/spdlog/")
find_package(spdlog REQUIRED)

add_executable(RadarSerial main.cpp RadarSerial.cpp)

target_precompile_headers(RadarSerial PRIVATE pch.h)

target_link_libraries(RadarSerial  -lpthread spdlog::spdlog)
