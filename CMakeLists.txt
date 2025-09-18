cmake_minimum_required(VERSION 3.10)
project(usb_map CXX)

# 设置 C++17
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_PREFIX_PATH "/home/mq-b/project/vcpkg/installed/x64-linux")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_definitions(SPDLOG_USE_STD_FORMAT)

# 可选：静态链接 libstdc++ 和 libgcc
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -pthread")

# 查找 libudev
find_library(LIBUDEV_LIBRARY udev)
if(NOT LIBUDEV_LIBRARY)
    message(FATAL_ERROR "libudev not found! Please install libudev-dev")
endif()

add_executable(usb_map src/usb_map.cpp)
add_executable(find_4g_module src/find_4g_module.cpp src/serial/serial.cpp)

find_package(spdlog CONFIG REQUIRED)

target_link_libraries(usb_map PRIVATE 
    ${LIBUDEV_LIBRARY}
    spdlog::spdlog_header_only
)
target_link_libraries(find_4g_module PRIVATE
    ${LIBUDEV_LIBRARY}
    spdlog::spdlog_header_only
)