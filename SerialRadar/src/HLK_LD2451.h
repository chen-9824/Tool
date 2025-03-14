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
    void stop_read_thread();
    void pause_read_thread();
    void resume_read_thread();

    std::vector<target_info> get_target_data();

    /*
    下发命令给雷达

    命令格式  帧头 | 帧内数据长度 | 帧内数据(命令字 | 命令值) | 帧尾
    回复格式  帧头 | 帧内数据长度 | 帧内数据((发送命令字 | 0x0100) | 返回值) | 帧尾

    cmd_key：命令字
    cmd_val：命令值
    return: 命令返回值（执行状态 + 其它参数）
    */
    std::vector<uint8_t> send_cmd(const std::vector<uint8_t> &cmd_key, const std::vector<uint8_t> &cmd_val);

    void enable_cfg_mode(); // 对雷达下发的任何其他命令必须在此命令下发后方可执行，否则无效
    void disable_cfg_mode();

    void read_target_cfg();

private:
    static void
    read_thread(int id, HLK_LD2451 *radar);
    void parse_hlk_radar_data(const std::vector<std::vector<uint8_t>> &data);

private:
    // std::shared_ptr<RadarSerial> _radar;

    std::thread _read_t;
    std::atomic<bool> _reading;
    std::atomic<bool> _pausing;

    std::mutex dataMutex;
    std::deque<struct target_info> dataQueue;
};

#endif