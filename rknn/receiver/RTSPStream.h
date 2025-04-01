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
#include <deque>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

class RTSPStream
{
public:
    enum player_type
    {
        none, // 不播放
        opencv,
        opengl
    };

    RTSPStream(const std::string &url, int dst_width, int dst_height, AVPixelFormat dst_fmt);
    ~RTSPStream();

    bool startPlayer(player_type type);
    void stop();

    void get_latest_frame(AVFrame &frame);

    void print_frame_timestamp(AVFrame *frame);

private:
    int ffmpeg_rtsp_init();
    void ffmpeg_rtsp_deinit();
    void streamLoop();
    bool is_frame_outdated(AVFrame *frame);
    bool is_pkt_outdated(AVPacket *packet);

private:
    std::string url_;
    std::atomic<bool> running_;
    std::thread stream_thread_;

    player_type _player_type = player_type::none;

    int _dst_width;
    int _dst_height;
    AVPixelFormat _dst_fmt;

    AVFormatContext *formatCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;
    AVFrame *frame_bgr = nullptr;
    AVFrame *latest_frame = nullptr;
    int videoStreamIndex = -1;
    SwsContext *swsCtx = nullptr;

    std::queue<AVFrame *> frameQueue;
    std::mutex frameQueueMutex;
    std::condition_variable frameQueueCond;

    int64_t startTime;
};

#endif