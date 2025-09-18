#include "serial.h"
#include <fcntl.h>
#include <spdlog/spdlog.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <cstring>
#include <stdexcept>
#include <sys/select.h>

Serial::Serial(const std::string& port, int baudrate, int timeout_ms)
    : fd_(-1), open_(false), timeoutMs_(timeout_ms)
{
    open(port, baudrate, timeout_ms);
}

Serial::Serial() : fd_(-1), open_(false), timeoutMs_(1000) 
{}

Serial::~Serial() {
    close();
}

void Serial::open(const std::string& port, int baudrate, int timeout_ms) {
    fd_ = ::open(port.c_str(), O_RDWR | O_NOCTTY | O_NDELAY);
    if (fd_ == -1) {
        spdlog::error("Failed to open serial port [{}] : {}", port, strerror(errno));
        throw std::runtime_error("Failed to open serial port: " + std::string(strerror(errno)));
    }

    fcntl(fd_, F_SETFL, 0);  // 设置为阻塞模式

    termios options;
    if (tcgetattr(fd_, &options) != 0) {
        throw std::runtime_error("tcgetattr failed");
    }

    speed_t baud;
    switch (baudrate) {
        case 115200: baud = B115200; break;
        case 9600:   baud = B9600; break;
        default:     throw std::runtime_error("Unsupported baudrate");
    }

    cfsetispeed(&options, baud);
    cfsetospeed(&options, baud);

    options.c_cflag |= (CLOCAL | CREAD);  // 启动接收器，忽略MODEM状态行
    options.c_cflag &= ~PARENB;
    options.c_cflag &= ~CSTOPB;
    options.c_cflag &= ~CSIZE;
    options.c_cflag |= CS8;

    options.c_cflag &= ~CRTSCTS;

    options.c_lflag &= ~(ICANON | ECHO | ECHOE | ISIG);
    options.c_oflag &= ~OPOST;

    options.c_cc[VMIN] = 1;
    options.c_cc[VTIME] = timeout_ms / 100;  // 单位是 1/10 秒

    if (tcsetattr(fd_, TCSANOW, &options) != 0) {
        throw std::runtime_error("tcsetattr failed");
    }

    open_ = true;
}

bool Serial::isOpen() const {
    return open_;
}

void Serial::close() {
    if (fd_ != -1) {
        // 创建一个分离的线程来执行关闭操作
        std::thread closeThread([fd = fd_]() {
            auto close_start = std::chrono::steady_clock::now();
            tcflush(fd, TCIOFLUSH);
            ::close(fd);
            auto close_end = std::chrono::steady_clock::now();
            auto close_duration = std::chrono::duration_cast<std::chrono::milliseconds>(close_end - close_start);
            //spdlog::info("Detached close completed in {} ms", close_duration.count());
        });
        
        // 分离线程，让它在后台独立运行
        closeThread.detach();
        
        // // 更新对象状态
        // fd_ = -1;
        // open_ = false;
    }
}

void Serial::write(const std::string& data) {
    tcflush(fd_, TCOFLUSH);  // 清空输出缓冲区
    if (!open_) throw std::runtime_error("Serial port not open");
    auto written = ::write(fd_, data.c_str(), data.size());
    if (written != data.size()) {
        spdlog::error("Failed to write to serial port");
        throw std::runtime_error("Failed to write to serial port");
    }
}

std::string Serial::readline(char delimiter) {
    if (!open_) throw std::runtime_error("Serial port not open");

    std::string line;
    char c;

    while (true) {
        ssize_t n = ::read(fd_, &c, 1);
        if (n == 1) {
            if (c == delimiter) break;
            line += c;
        } else if (n == 0) {
            // 没数据，可以 sleep 一下避免死循环
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        } else {
            break; // 出错
        }
    }
    return line;
}

std::string Serial::readAll(){
    constexpr std::size_t bufferSize = 1024; // 定义缓冲区大小
    char tempBuffer[bufferSize]{};           // 使用临时缓冲区
    
    // 使用select实现超时机制
    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd_, &readfds);
    
    timeval timeout;
    timeout.tv_sec = timeoutMs_ / 1000;           // 秒
    timeout.tv_usec = (timeoutMs_ % 1000) * 1000; // 微秒
    
    int result = select(fd_ + 1, &readfds, nullptr, nullptr, &timeout);
    
    if (result > 0 && FD_ISSET(fd_, &readfds)) {
        // 数据可读
        int n = ::read(fd_, tempBuffer, bufferSize - 1);
        if (n > 0) {
            tempBuffer[n] = '\0';  // 确保字符串结尾
            //spdlog::info("read raw: {}", tempBuffer);
            return std::string(tempBuffer);
        }
    } else if (result == 0) {
        // 超时
        //spdlog::info("Read timeout");
        return std::string();
    } else {
        // 错误
        spdlog::error("Read error: {}", strerror(errno));
    }
    
    return std::string(); // 返回空字符串
}

int Serial::fd(){
    return fd_;
}