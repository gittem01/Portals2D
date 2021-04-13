cmake_minimum_required(VERSION 3.16.3)
project(Teleporter VERSION 1.0.0)

set (CMAKE_CXX_STANDARD 17)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/box2d/src)
add_subdirectory(/home/eight/Documents/dev/SDL ./SDL)

set(GLAD_DIR "${CMAKE_SOURCE_DIR}/external/glad")

include_directories(${CMAKE_SOURCE_DIR}/mines)
include_directories(${CMAKE_SOURCE_DIR}/external/glad/include)
include_directories(${CMAKE_SOURCE_DIR}/external/glad/include/glad)
include_directories(${CMAKE_SOURCE_DIR}/external/glfw/include)
include_directories(${CMAKE_SOURCE_DIR}/external/box2d/include)
include_directories(/home/eight/Documents/dev/SDL/include)

link_directories(./build/SDL)

add_executable(physicsTeleport Source.cpp)

add_library(glad 
            ${GLAD_DIR}/src/glad.c
)

file(GLOB files ${CMAKE_SOURCE_DIR}/mines/*.cpp)
add_library(mines ${files})

find_package(OpenGL REQUIRED)
target_link_libraries(physicsTeleport OpenGL::GL)

target_link_libraries(physicsTeleport mines)
target_link_libraries(physicsTeleport glad)
target_link_libraries(physicsTeleport box2d)
target_link_libraries(physicsTeleport libSDL2-2.0.so)
