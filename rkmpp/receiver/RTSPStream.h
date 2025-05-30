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
#include <chrono>
#include <iomanip>

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

    virtual bool startPlayer(player_type type);
    virtual void stop();

    void get_latest_frame(AVFrame &frame);
    AVFrame *get_latest_frame();

    void print_frame_timestamp(AVFrame *frame);
    void print_elapsed_time(const std::chrono::high_resolution_clock::time_point &start);
    void print_elapsed_time();

    std::string get_frame_timestamp(AVFrame *frame);
    std::string get_elapsed_time();

protected:
    std::atomic<bool> running_;
    std::thread stream_thread_;

    std::string url_;
    AVFormatContext *formatCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVPacket *packet = nullptr;
    int videoStreamIndex = -1;

    int max_error_frame_count = 10;

    int64_t startTime;

private:
    int ffmpeg_rtsp_init();
    void ffmpeg_rtsp_deinit();
    void streamLoop();
    bool is_frame_outdated(AVFrame *frame);
    bool is_pkt_outdated(AVPacket *packet);
    void push_frame(AVFrame *frame);

private:
    player_type _player_type = player_type::none;

    int _dst_width;
    int _dst_height;
    AVPixelFormat _dst_fmt;

    AVFrame *frame = nullptr;
    AVFrame *frame_bgr = nullptr;
    AVFrame *latest_frame = nullptr;
    uint8_t *yuv_buffer = nullptr;
    uint8_t *bgr_buffer = nullptr;
    uint8_t *lates_buffer = nullptr;
    SwsContext *swsCtx = nullptr;

    std::queue<AVFrame *> frameQueue;
    std::mutex frameQueueMutex;
    std::condition_variable frameQueueCond;
};

#endif