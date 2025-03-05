cmake_minimum_required(VERSION 3.5.0)
project(FolderMonitor VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/spdlog/lib/cmake/spdlog/")
find_package(spdlog REQUIRED)

add_executable(FolderMonitor main.cpp FolderMonitor.cpp)

target_link_libraries(FolderMonitor -lpthread spdlog::spdlog)

