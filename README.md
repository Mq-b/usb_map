# usb_map

Linux 下 USB 串口设备映射查询与 4G 模块检测工具集。

## 程序

### usb_map

遍历 `/dev` 下的串口设备以及 `ttyUSB*` / `ttyACM*` 物理设备，通过 libudev 查询其 USB 接口 ID 并展示映射关系。

```txt
用法: usb_map [--links | --phys | --all]
  --links  仅显示虚拟设备（符号链接）
  --phys   仅显示物理设备
  --all    显示全部（默认）
```

生成 udev 规则文件，实现设备名持久化映射：

```
用法: usb_map --add <虚拟名称> <物理ID> [--file <文件名>]
  --add    添加或更新 udev 规则
  --file   规则文件名（默认: /etc/udev/rules.d/relia.rules）
```

示例：

```bash
# 查看当前设备映射
usb_map --phys

# 为物理设备 1-1 创建符号链接 ttyCAN（写入默认路径）
usb_map --add ttyCAN 1-1

# 指定规则文件名
usb_map --add ttyLIS 1-4.5:1.0 --file /etc/udev/rules.d/device.rules
```

若规则文件中已存在相同物理 ID 的规则，则更新；否则追加。规则格式：

```
KERNEL=="ttyUSB*", KERNELS=="<物理ID>", MODE:="0664", SYMLINK+="<虚拟名称>"
```

`SYMLINK+="<虚拟名称>"` 没有绝对路径时，udev 自动在 `/dev/` 下创建符号链接。

将规则文件放到 `/etc/udev/rules.d/` 后重载 udev 即可生效：

```bash
sudo udevadm control --reload-rules
sudo udevadm trigger
```

或直接重启：`sudo reboot`

![](./images/usb_map-example.png)

### find_4g_module

遍历 `/dev/ttyUSB0-9`，逐个发送 `AT\r\n` 指令，通过响应中是否包含 `OK` 来定位移远 4G 模块所在的串口。

```txt
用法: find_4g_module
```

## 依赖

| 依赖 | Ubuntu 包名 |
|------|-------------|
| CMake ≥ 3.10 | `cmake` |
| libudev | `libudev-dev` |
| spdlog | `libspdlog-dev` |
| g++ (C++20) | `build-essential` |

## 构建

```bash
# 安装依赖
sudo apt install -y build-essential cmake libudev-dev libspdlog-dev

# 编译
cmake -B build -DCMAKE_PREFIX_PATH=/usr
cmake --build build
```

输出在 `build/bin/` 目录下。

> 若使用 vcpkg 管理 spdlog，请将 `CMAKE_PREFIX_PATH` 指向 vcpkg 安装路径。
