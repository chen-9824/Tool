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

    void setReconnect(int dup_ms, bool start_read = false);

    bool is_available();
    bool is_reading();

    void set_frame_flag(uint8_t frame_start_flag, uint8_t frame_end_flag);
    void set_frame_flag(int frame_min_size, std::vector<uint8_t> frame_header, std::vector<uint8_t> frame_tail);

    void startReading();
    void stopReading();
    void pauseReading();
    void resumeReading();

    bool sendCommand(const std::vector<uint8_t> &cmd);
    std::vector<std::vector<uint8_t>> getAllData();
    std::vector<uint8_t> getLatestData();
    std::vector<uint8_t> getOldestData();

    /*
    裁剪buffer, 去掉frame_header 两字节帧长度 frame_tail 只保留帧内容返回
    注意：该函数将修改buffer
    */
    std::vector<std::vector<uint8_t>> cutFrame(std::vector<uint8_t> &buffer, const std::vector<uint8_t> &frame_header, const std::vector<uint8_t> &frame_tail, int complete_frame_size);
    std::vector<std::vector<uint8_t>> parseFrame(std::vector<uint8_t> &buffer);

    void printf_uint8(const std::vector<uint8_t> &data);
    void printf_str(const std::vector<uint8_t> &data);

private:
    void readLoop();
    void startConnecting();
    void stopConnecting();
    void connectLoop();
    bool is_serial_existed();

protected:
private:
    bool _available = false;
    int fd;
    std::string port;
    int baudrate;
    std::atomic<bool> reading;
    std::atomic<bool> pausing;

    std::thread readThread;
    std::mutex ioMutex;
    std::mutex dataMutex;
    std::deque<std::vector<uint8_t>> dataQueue;

    std::thread connectThread;
    std::atomic<bool> reconnect_enable;
    int reconnect_dup;      // ms
    bool start_read_enable; // 重连成功之后是否启动读线程

    bool _complete_frame_enable = false; // 是否拼为完整一帧存储
    uint8_t _frame_start_flag = 'N';     // 帧起始字符
    uint8_t _frame_end_flag = '\n';      // 帧结束字符

    int _frame_min_size = 17; // 至少包含一个目标数据
    std::vector<uint8_t> _frame_header{0xf4, 0xf3, 0xf2, 0xf1};
    std::vector<uint8_t> _frame_tail{0xf8, 0xf7, 0xf6, 0xf5};
};

#endif