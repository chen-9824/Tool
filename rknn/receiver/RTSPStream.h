#ifndef RTSPSTREAM_H
#define RTSPSTREAM_H

#pragma once

#include <iostream>
#include <string>
#include <thread>
#include <atomic>
#include <queue>
#include <mutex>
#include <condition_variable>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

class RTSPStream
{
public:
    RTSPStream(const std::string &url, int dst_width, int dst_height, AVPixelFormat dst_fmt);
    ~RTSPStream();

    bool start();
    void stop();

private:
    void streamLoop();

    int _dst_width;
    int _dst_height;
    AVPixelFormat _dst_fmt;

    std::string url_;
    std::atomic<bool> running_;
    std::thread stream_thread_;
};

#endif