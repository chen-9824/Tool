cmake_minimum_required(VERSION 3.12)

add_library(utils_obj OBJECT "")

SET(THIRD_PARTY_PATH ${CMAKE_CURRENT_LIST_DIR}/3rdparty)
SET(UTILS_LIBRGA_SOURCE_PATH ${CMAKE_CURRENT_LIST_DIR}/../..)

include(${CMAKE_CURRENT_LIST_DIR}/allocator/CMakeLists.txt)
include(${CMAKE_CURRENT_LIST_DIR}/3rdparty/CMakeLists.txt)

set(UTILS_SRC
    ${CMAKE_CURRENT_LIST_DIR}/utils.cpp
)

if(RGA_SOURCE_CODE_TYPE STREQUAL c)
    set_source_files_properties(${UTILS_SRC} PROPERTIES LANGUAGE C COMPILE_FLAGS "-x c")
endif()

target_sources(utils_obj
    PRIVATE
        ${UTILS_SRC}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/utils.h
)

target_include_directories(utils_obj
    PRIVATE
        ${UTILS_LIBRGA_SOURCE_PATH}/include
         ${UTILS_LIBRGA_SOURCE_PATH}/../../rknn_yolov5/include
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)

SET(RGA_SAMPLES_UTILS_COMPILED "true" CACHE STRING "")
