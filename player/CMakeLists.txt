cmake_minimum_required(VERSION 3.10.0)
project(player VERSION 0.1.0 LANGUAGES C CXX)
#播放器

include_directories(${CMAKE_SOURCE_DIR}/opengl/glad/include)
add_library(glad ${CMAKE_SOURCE_DIR}/opengl/glad/src/glad.c)


include_directories(${CMAKE_SOURCE_DIR}/opengl/)

# 查找OpenCV库
find_package(OpenCV REQUIRED)

# 包含OpenCV头文件
include_directories(${OpenCV_INCLUDE_DIRS})
LINK_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR}/lib)

add_executable(player player.cpp ${CMAKE_CURRENT_SOURCE_DIR}/opengl/Shader.cpp ${CMAKE_CURRENT_SOURCE_DIR}/opengl/VideoRenderer.cpp)

target_link_libraries(player ${OpenCV_LIBS} -lglfw glad -ldl)
