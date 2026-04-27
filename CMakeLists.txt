cmake_minimum_required(VERSION 3.10)
project(usb_map CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 查找 libudev
find_library(LIBUDEV_LIBRARY udev)
if(NOT LIBUDEV_LIBRARY)
    message(FATAL_ERROR "libudev not found! Please install libudev-dev")
endif()

add_executable(usb_map src/usb_map.cpp)
add_executable(find_4g_module src/find_4g_module.cpp src/serial/serial.cpp)

target_link_libraries(usb_map PRIVATE ${LIBUDEV_LIBRARY})

find_package(spdlog CONFIG REQUIRED)
target_compile_definitions(find_4g_module PRIVATE SPDLOG_USE_STD_FORMAT)
target_link_libraries(find_4g_module PRIVATE ${LIBUDEV_LIBRARY} spdlog::spdlog_header_only)

# 安装
include(GNUInstallDirs)
install(TARGETS usb_map find_4g_module RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
