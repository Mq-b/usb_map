#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <dirent.h>
#include <libudev.h>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>

// 判断是否符号链接
bool is_symlink(const std::string& path) {
    struct stat sb{};
    if (lstat(path.c_str(), &sb) == -1) 
        return false;
    else
        return S_ISLNK(sb.st_mode);
}

// 判断路径是否存在
bool file_exists(const std::string& path) {
    struct stat buffer{};
    return (stat(path.c_str(), &buffer) == 0);
}

// 解析符号链接
std::string resolve_symlink(const std::string& path) {
    char buf[PATH_MAX];
    ssize_t len = readlink(path.c_str(), buf, sizeof(buf) - 1);
    if (len != -1) {
        buf[len] = '\0';
        char real[PATH_MAX];
        if (realpath(buf, real)) {
            return std::string(real);
        }
        return std::string(buf);
    }
    return path;
}

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

// 打印一行
void print_row(const std::string& vdev, const std::string& pdev, const std::string& interfaceId) {
    std::cout << " " << vdev<< "\t\t" << pdev  << "\t\t" << interfaceId << std::endl;
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
    std::cout << " 虚拟设备" << "\t" << "物理设备" << "\t" << "接口ID" << std::endl;

    // 遍历符号链接设备
    if (showLinks) {
        DIR* dir = opendir("/dev");
        if (dir) {
            dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                std::string vdev = "/dev/" + std::string(entry->d_name);

                if (vdev.find("/dev/tty") != 0) continue;
                if (!is_symlink(vdev)) continue;

                std::string pdev = resolve_symlink(vdev);
                if (pdev.find("ttyUSB") == std::string::npos && pdev.find("ttyACM") == std::string::npos)
                    continue;

                std::string interfaceId = get_interface_id(udev, pdev);
                print_row(vdev.substr(5), pdev.substr(pdev.find_last_of("/") + 1), interfaceId);
            }
            closedir(dir);
        }
    }

    // 遍历物理设备
    if (showPhys) {
        for (const std::string& prefix : {"/dev/ttyUSB", "/dev/ttyACM"}) {
            for (int i = 0; i < 32; i++) {
                std::string devPath = prefix + std::to_string(i);
                if (!file_exists(devPath)) continue;

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