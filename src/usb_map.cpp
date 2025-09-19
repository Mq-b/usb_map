#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <filesystem>
#include <dirent.h>
#include <libudev.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
namespace fs = std::filesystem;

// 获取接口 ID
std::string get_interface_id(struct udev* udev, const std::string& devPath) {
    std::string basename = devPath.substr(devPath.find_last_of("/") + 1);
    struct udev_device* dev = udev_device_new_from_subsystem_sysname(udev, "tty", basename.c_str());
    if (!dev) return "N/A";

    struct udev_device* parent = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");
    std::string interfaceId = "N/A";
    if (parent) {
        const char* sysname = udev_device_get_sysname(parent);
        if (sysname) interfaceId = sysname;
    }

    udev_device_unref(dev);
    return interfaceId;
}

void print_row(const std::string& vdev, const std::string& pdev, const std::string& interfaceId) {
    std::cout << " " << std::left << std::setw(20) << vdev 
              << std::left << std::setw(20) << pdev 
              << std::left << std::setw(20) << interfaceId << std::endl;
}

// 遍历符号链接设备
void print_symlink_devices(struct udev* udev) {
    for (const auto& entry : fs::directory_iterator("/dev")) {
        if (!entry.is_symlink()) continue;
        std::string vdev = entry.path().string();
        std::string filename = entry.path().filename().string();
        if (filename.find("tty") != 0) continue;

        std::error_code ec;
        fs::path resolved = fs::read_symlink(entry.path(), ec);
        if (ec) continue;
        std::string pdev = resolved.is_absolute() ? resolved.string() : (entry.path().parent_path() / resolved).string();
        std::string pdev_name = fs::path(pdev).filename().string();

        if (pdev_name.find("ttyUSB") == std::string::npos && pdev_name.find("ttyACM") == std::string::npos)
            continue;

        std::string interfaceId = get_interface_id(udev, pdev);
        print_row(vdev.substr(5), pdev_name, interfaceId);
    }
}

// 遍历物理设备
void print_phys_devices(struct udev* udev) {
    for (const std::string& prefix : {"/dev/ttyUSB", "/dev/ttyACM"}) {
        for (int i = 0; i < 32; i++) {
            std::string devPath = prefix + std::to_string(i);
            if (!fs::exists(devPath)) continue;

            std::string interfaceId = get_interface_id(udev, devPath);
            print_row("-", devPath.substr(5), interfaceId);
        }
    }
}

void parse_command_line_args(int argc, char* argv[], bool& showLinks, bool& showPhys) {
    if (argc > 1) {
        std::string arg = argv[1];
        if (arg == "--links") {
            showLinks = true;
            showPhys = false;
        } else if (arg == "--phys") {
            showLinks = false;
            showPhys = true;
        } else if (arg == "--all") {
            showLinks = true;
            showPhys = true;
        } else {
            std::cerr << "用法: " << argv[0] << " [--links | --phys | --all]" << std::endl;
            exit(1);
        }
    }
}

int main(int argc, char* argv[]) {
    bool showLinks = true;
    bool showPhys = true;

    parse_command_line_args(argc, argv, showLinks, showPhys);

    udev* udev = udev_new();
    if (!udev) {
        std::cerr << "无法创建 udev 上下文" << std::endl;
        return 1;
    }

    std::cout << "虚拟串口设备映射关系:" << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    // 使用固定宽度对齐
    std::cout << " " << std::left << std::setw(20) << "虚拟设备" 
              << std::left << std::setw(20) << "   物理设备" 
              << std::left << std::setw(20) << "        接口ID" << std::endl;

    if (showLinks) print_symlink_devices(udev);
    if (showPhys) print_phys_devices(udev);
}
/*
g++ -std=c++17 ./src/usb_map.cpp -o ./build/usb_map -ludev -pthread -static-libstdc++ -static-libgcc
*/