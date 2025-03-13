#ifndef HLK_LD2451_H
#define HLK_LD2451_H

#pragma once

#include "RadarSerial.h"

class HLK_LD2451 : public RadarSerial
{
public:
    struct target_info
    {
        int angle;
        int distance;  // m
        int direction; // 1:靠近 0:远离
        int speed;     // km/h
        int SNR;       // 信噪比
    };

public:
    HLK_LD2451(const std::string &port, int baudrate = B115200);
    ~HLK_LD2451();

    int init();
    void deinit();

    void print_target_info(const target_info &data);

    void start_read_thread();

    std::vector<target_info> get_target_data();

private:
    static void read_thread(int id, HLK_LD2451 *radar);
    void parse_hlk_radar_data(const std::vector<std::vector<uint8_t>> &data);

private:
    std::shared_ptr<RadarSerial> _radar;
    bool _available = false;

    std::thread _read_t;
    std::atomic<bool> _reading;

    std::mutex dataMutex;
    std::deque<struct target_info> dataQueue;
};

#endif