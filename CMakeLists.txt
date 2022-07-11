cmake_minimum_required(VERSION 3.0)
project(Teleporter VERSION 2.0.0)

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DGL_SLIENCE_DEPRECATION)

add_subdirectory(${CMAKE_SOURCE_DIR}/external/box2d/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw ${CMAKE_SOURCE_DIR}/build/glfw)


include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/helpers/include)
include_directories(${CMAKE_SOURCE_DIR}/external/glm/include)
include_directories(${CMAKE_SOURCE_DIR}/external/box2d/include)
include_directories(${CMAKE_SOURCE_DIR}/external/glfw/include)

link_directories(${CMAKE_SOURCE_DIR}/build/glfw)

file(GLOB HELPER_SOURCES ${CMAKE_SOURCE_DIR}/helpers/src/*.cpp)
file(GLOB HELPER_HEADERS ${CMAKE_SOURCE_DIR}/helpers/include/*.h)

add_executable(physicsTeleport Source.cpp ${HELPER_SOURCES} ${HELPER_HEADERS})

file(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.h)
add_library(PORTAL2D ${SOURCE_FILES} ${HEADER_FILES})

find_package(OpenGL REQUIRED)
target_link_libraries(physicsTeleport OpenGL::GL)

target_link_libraries(physicsTeleport PORTAL2D)
target_link_libraries(physicsTeleport box2d)
target_link_libraries(physicsTeleport glfw)
