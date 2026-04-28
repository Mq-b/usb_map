# AGENTS.md

## Build Commands

```bash
# Install dependencies (Ubuntu)
sudo apt install -y build-essential cmake libudev-dev libspdlog-dev

# Configure and build
cmake -B build -DCMAKE_PREFIX_PATH=/usr
cmake --build build

# Binaries output to build/bin/
```

## Project Structure

- Two independent C++20 executables, no shared libraries:
  - `usb_map` (src/usb_map.cpp) — USB serial device mapping via libudev
  - `find_4g_module` (src/find_4g_module.cpp + src/serial/serial.cpp) — 4G module detection via AT commands
- CI: Ubuntu 22.04 and 24.04 only

## Key Facts

- **Linux-only**: uses `/dev`, `libudev`, and POSIX serial APIs — will not build on Windows/macOS
- **spdlog with std::format**: `find_4g_module` defines `SPDLOG_USE_STD_FORMAT`; ensure spdlog is built with std format support
- **Serial close can block ~30s**: `serial.close()` in find_4g_module may hang (see src/find_4g_module.cpp:46)
- **No tests**: no test targets in CMakeLists.txt; verification is manual (run on Linux with USB devices)
- **No install prefix needed**: cmake configure uses `-DCMAKE_PREFIX_PATH=/usr` for system spdlog

## udev Rules

`usb_map --add` generates udev rules for persistent device naming. Rules target `/etc/udev/rules.d/` and follow this pattern:

```
KERNEL=="ttyUSB*", KERNELS=="<physical_id>", MODE:="0664", SYMLINK+="<virtual_name>"
```

- `KERNELS` matches the USB port topology path (e.g. `1-1`, `1-4.5:1.0`) — use `usb_map --phys` to discover these
- `SYMLINK` creates a stable `/dev/<name>` symlink regardless of enumeration order
- Rule files are matched and updated by `KERNELS` value — re-running with the same physical ID updates in place
