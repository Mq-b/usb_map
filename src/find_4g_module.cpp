#include "serial/serial.h"
#include <spdlog/spdlog.h>
#include <filesystem>
#include <libudev.h>

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

int main(){
    Serial serial;
    std::string result_port;
    std::string command = "AT\r\n";
    // 遍历所有USB设备，读取AT响应OK
    for(int i = 0; i < 10; i++){
        std::string port = "/dev/ttyUSB" + std::to_string(i);
        try{
            if(!std::filesystem::exists(port)) continue;
            //spdlog::info("Trying {}", port);
            serial.open(port, 115200);
            serial.write(command);
            //spdlog::info("Sent command to {}", port);
            std::string response = serial.readAll();
            if(response.find("OK") != std::string::npos){
                //spdlog::info("Found 4G module at {}", port);
                result_port = port;
            }
        } catch(const std::exception& e){
            spdlog::error("Error opening {}: {}", port, e.what());
            serial.close();
        }
        //spdlog::info("Closing {}", port);
        // close 可能导致阻塞30秒
        serial.close();
        //spdlog::info("Closed2 {}", port);
    }
    
    udev* udev = udev_new();
    std::string interfaceId = get_interface_id(udev, result_port);
    udev_unref(udev);
    spdlog::info("Found 4G module at {}, interface ID: {}", result_port, interfaceId);
}