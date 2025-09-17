cmake_minimum_required(VERSION 3.10)
project(usb_map CXX)

# 设置 C++17
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/build)

# 可选：静态链接 libstdc++ 和 libgcc
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++ -pthread")

# 查找 libudev
find_library(LIBUDEV_LIBRARY udev)
if(NOT LIBUDEV_LIBRARY)
    message(FATAL_ERROR "libudev not found! Please install libudev-dev")
endif()


add_executable(${PROJECT_NAME} src/usb_map.cpp)


target_link_libraries(${PROJECT_NAME} PRIVATE ${LIBUDEV_LIBRARY})
