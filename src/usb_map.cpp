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
    std::cout << " " << vdev<< "\t\t\t" << pdev  << "\t\t\t" << interfaceId << std::endl;
}

int main(int argc, char* argv[]) {
    bool showLinks = true;
    bool showPhys = true;

    // 命令行选项
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
            return 1;
        }
    }

    udev* udev = udev_new();
    if (!udev) {
        std::cerr << "无法创建 udev 上下文" << std::endl;
        return 1;
    }

    std::cout << "虚拟串口设备映射关系:" << std::endl;
    std::cout << "--------------------------------------------------------------" << std::endl;
    std::cout << " 虚拟设备" << "\t\t" << "物理设备" << "\t\t" << "接口ID" << std::endl;

    // 遍历符号链接设备
    if (showLinks) {
        for (const auto& entry : fs::directory_iterator("/dev")) {
            if (!entry.is_symlink()) continue;
            // 获取符号链接设备名称
            std::string vdev = entry.path().string();
            // 获取符号链接设备名称（不带 /dev 前缀）
            std::string filename = entry.path().filename().string();
            // 跳过非 tty 设备
            if (filename.find("tty") != 0) continue;

            // 获取符号链接设备对应的物理设备名称
            std::error_code ec;
            fs::path resolved = fs::read_symlink(entry.path(), ec);
            if (ec) continue;
            
            // 将解析出的物理设备路径转换为绝对路径：如果resolved已经是绝对路径则直接使用，否则将其与符号链接所在目录组合成完整路径
            std::string pdev = resolved.is_absolute() ? resolved.string() : (entry.path().parent_path() / resolved).string();

            // 从完整路径中提取设备文件名（去掉目录部分），例如从"/dev/ttyUSB0"提取出"ttyUSB0"
            std::string pdev_name = fs::path(pdev).filename().string();

            if (pdev_name.find("ttyUSB") == std::string::npos && pdev_name.find("ttyACM") == std::string::npos)
                continue;

            std::string interfaceId = get_interface_id(udev, pdev);
            print_row(vdev.substr(5), pdev_name, interfaceId);
        }
    }

    // 遍历物理设备
    if (showPhys) {
        for (const std::string& prefix : {"/dev/ttyUSB", "/dev/ttyACM"}) {
            for (int i = 0; i < 32; i++) {
                std::string devPath = prefix + std::to_string(i);
                if (!fs::exists(devPath)) continue;

                std::string interfaceId = get_interface_id(udev, devPath);
                print_row("-", devPath.substr(5), interfaceId);
            }
        }
    }

    udev_unref(udev);
}
/*
g++ -std=c++17 ./src/usb_map.cpp -o ./build/usb_map -ludev -pthread -static-libstdc++ -static-libgcc
*/