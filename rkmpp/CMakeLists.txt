cmake_minimum_required(VERSION 3.10.0)
project(RKMPP VERSION 0.1.0 LANGUAGES C CXX)

add_executable(RKMPP main.cpp RkMpp.cpp util.cpp)

target_include_directories(RKMPP PRIVATE mpp/include/rockchip/)
target_link_directories(RKMPP PRIVATE mpp/lib)

target_link_libraries(RKMPP rockchip_mpp)

set_target_properties(RKMPP PROPERTIES
    BUILD_RPATH "mpp/lib"
)