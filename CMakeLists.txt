cmake_minimum_required(VERSION 3.0)
project(Teleporter VERSION 2.0.0)

set(APP_NAME PhysicsTeleport)
set(CMAKE_CXX_STANDARD 17)

add_definitions(-DGL_SLIENCE_DEPRECATION)

include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/helpers/include)
include_directories(${CMAKE_SOURCE_DIR}/helpers/include/vkInclude)
include_directories(${CMAKE_SOURCE_DIR}/helpers/include/vkInclude/ShaderHeaders)
include_directories(${CMAKE_SOURCE_DIR}/helpers/TestCases)
include_directories(${CMAKE_SOURCE_DIR}/external/glm/include)
include_directories(${CMAKE_SOURCE_DIR}/external/box2d/include)
include_directories(${CMAKE_SOURCE_DIR}/external/glfw/include)
include_directories(${CMAKE_SOURCE_DIR}/external/tiny_gltf)
include_directories(${CMAKE_SOURCE_DIR}/external/imgui)
include_directories(${CMAKE_SOURCE_DIR}/external/vulkan/include)
include_directories(${CMAKE_SOURCE_DIR}/external/OpenAlSoft/include)
include_directories(${CMAKE_SOURCE_DIR}/external/AudioFile)
include_directories(${CMAKE_SOURCE_DIR}/external/stb_truetype)


add_subdirectory(${CMAKE_SOURCE_DIR}/external/box2d/src)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/glfw ${CMAKE_SOURCE_DIR}/build/glfw)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/imgui)
add_subdirectory(${CMAKE_SOURCE_DIR}/external/OpenALSoft)

file(GLOB HELPER_SOURCES    ${CMAKE_SOURCE_DIR}/helpers/src/*.cpp
                            ${CMAKE_SOURCE_DIR}/helpers/src/vkSource/*.cpp
                            ${CMAKE_SOURCE_DIR}/helpers/src/vkSource/ShaderSources/*.cpp
                            ${CMAKE_SOURCE_DIR}/helpers/src/DefaultObjects/*.cpp)
                            
file(GLOB HELPER_HEADERS    ${CMAKE_SOURCE_DIR}/helpers/include/*.h
                            ${CMAKE_SOURCE_DIR}/helpers/include/vkInclude*.h
                            ${CMAKE_SOURCE_DIR}/helpers/TestCases/*.h)

set(   
    IMGUI_BACKENDS  
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_vulkan.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_vulkan.h
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.cpp
    ${CMAKE_SOURCE_DIR}/external/imgui/backends/imgui_impl_glfw.h)

link_directories(${CMAKE_SOURCE_DIR}/build/glfw)
# put this before add_executable else get linker error
link_directories(${CMAKE_SOURCE_DIR}/external/vulkan/lib)

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE portal.ico)
    set(appIcon ${CMAKE_CURRENT_SOURCE_DIR}/icon/${MACOSX_BUNDLE_ICON_FILE})
    set_source_files_properties(${appIcon} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    add_executable(${APP_NAME} MACOSX_BUNDLE Source.cpp ${HELPER_SOURCES} ${HELPER_HEADERS} ${IMGUI_BACKENDS} ${appIcon})
    file(COPY ${CMAKE_SOURCE_DIR}/assets DESTINATION ${APP_NAME}.app/Contents/Resources)
elseif(MSVC)
    #add_executable(${APP_NAME} WIN32 Source.cpp ${HELPER_SOURCES} ${HELPER_HEADERS} ${IMGUI_BACKENDS} ${CMAKE_CURRENT_SOURCE_DIR}/icon/portal.rc)
    add_executable(${APP_NAME} Source.cpp ${HELPER_SOURCES} ${HELPER_HEADERS} ${IMGUI_BACKENDS})
    #add_definitions(-DENABLE_WINMAIN)
else()
    add_executable(${APP_NAME} Source.cpp ${HELPER_SOURCES} ${HELPER_HEADERS} ${IMGUI_BACKENDS})
endif()

file(GLOB SOURCE_FILES ${CMAKE_SOURCE_DIR}/src/*.cpp)
file(GLOB HEADER_FILES ${CMAKE_SOURCE_DIR}/include/*.h)

add_library(PORTAL2D ${SOURCE_FILES} ${HEADER_FILES})
add_library(imgui ${IMGUI_SOURCES} ${IMGUI_HEADERS})

target_link_libraries(${APP_NAME} PORTAL2D)
target_link_libraries(${APP_NAME} box2d)
target_link_libraries(${APP_NAME} glfw)
target_link_libraries(${APP_NAME} imgui)
target_link_libraries(${APP_NAME} OpenAL)

if (APPLE)
	target_link_libraries(${APP_NAME} libvulkan.1.dylib)
elseif (UNIX)
	target_link_libraries(${APP_NAME} vulkanUnx)
elseif (WIN32)
	target_link_libraries(${APP_NAME} vulkanWin)
endif()