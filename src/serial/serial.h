#pragma once

#include <string>
#include <stdexcept>
#include <vector>

class Serial {
public:
    Serial(const std::string& port, int baudrate, int timeout_ms = 1000);
    Serial();
    ~Serial();

    void open(const std::string& port, int baudrate, int timeout_ms = 1000);
    bool isOpen() const;
    void close();
    void write(const std::string& data);
    std::string readline(char delimiter = '\n');
    std::string readAll();
    int fd();

private:
    int fd_;
    bool open_;
    int timeoutMs_;
};
