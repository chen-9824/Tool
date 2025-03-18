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
  bool push_frame(const YUVFrame &frame);
  bool push_frame(unsigned char *frame);
  void cleanup();

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
};

#endif