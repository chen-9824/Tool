#ifndef FFMPEGRTSPSTREAMER_H
#define FFMPEGRTSPSTREAMER_H

#pragma once

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>
}

#include <cstring>
#include <iostream>

struct YUVFrame
{
  unsigned char *data;
  int width;
  int height;
};

class FFmpegRTSPStreamer
{
public:
  FFmpegRTSPStreamer(const std::string &rtsp_url, int width, int height,
                     int fps);
  ~FFmpegRTSPStreamer();
  bool init();

  // 传入帧格式需为yuv420p
  bool push_frame(unsigned char *frame);
  void cleanup();

  // 右上角添加时间
  void add_time();

  void YUV422ToYUV420p(const unsigned char *yuv422, unsigned char *yuv420,
                       int width, int height);
  void YUV422ToYUV420pBySWS(const uint8_t *yuv422, uint8_t *yuv420, int width,
                            int height);

  void SaveYUV420pToFile(const uint8_t *yuv420p, int width, int height,
                         const std::string &filename);
  void SaveYUV422pToFile(const uint8_t *yuv422p, int width, int height,
                         const std::string &filename);
  void ConvertYUYVToYUV420P(const unsigned char *yuyv, unsigned char *yuv420p,
                            int width, int height);

private:
  void print_error(const std::string &msg, int errnum);
  int init_filters(const char *filters_descr);

private:
  std::string rtsp_url;
  int width;
  int height;
  int fps;
  unsigned long long frame_count = 0;

  AVFormatContext *fmt_ctx = nullptr;
  AVStream *video_stream = nullptr;
  AVCodecContext *codec_ctx = nullptr;
  AVCodec *codec = nullptr;
  AVFrame *av_frame = nullptr;
  SwsContext *sws_ctx_422_to_420 = nullptr;

  bool _filter_enable = false;
  AVFilterContext *buffersink_ctx;
  AVFilterContext *buffersrc_ctx;
  AVFilterGraph *filter_graph;
  AVFrame *av_filter_frame = nullptr;
};

#endif