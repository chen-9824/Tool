#ifndef RADARSERIAL_H
#define RADARSERIAL_H

#pragma once

#include <vector>
#include <thread>
#include <atomic>
#include <mutex>
#include <deque>
#include <termios.h>
class RadarSerial
{
public:
    RadarSerial(const std::string &port, int baudrate = B115200);
    ~RadarSerial();

    bool openSerial();
    void closeSerial();

    void set_frame_flag(uint8_t frame_start_flag, uint8_t frame_end_flag);

    void startReading();
    void stopReading();

    bool sendCommand(const std::vector<uint8_t> &cmd);
    std::vector<std::vector<uint8_t>> getAllData();
    std::vector<uint8_t> getLatestData();

    void parseData(const std::vector<uint8_t> &data);

private:
    void readLoop();

private:
    int fd;
    std::string port;
    int baudrate;
    std::atomic<bool> running;
    std::thread readThread;
    std::mutex ioMutex;
    std::mutex dataMutex;
    std::deque<std::vector<uint8_t>> dataQueue;

    bool _complete_frame_enable = false; // 是否拼为完整一帧存储
    uint8_t _frame_start_flag = 'N';     // 帧起始字符
    uint8_t _frame_end_flag = '\n';      // 帧结束字符
};

#endif