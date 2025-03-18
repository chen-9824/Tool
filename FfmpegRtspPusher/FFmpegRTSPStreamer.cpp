#include "FFmpegRTSPStreamer.h"

#include <chrono>
#include <fstream>
#include <vector>

#define DEBUG 0

FFmpegRTSPStreamer::FFmpegRTSPStreamer(const std::string &rtsp_url, int width,
                                       int height, int fps)
    : rtsp_url(rtsp_url),
      width(width),
      height(height),
      fps(fps),
      fmt_ctx(nullptr),
      video_stream(nullptr),
      codec_ctx(nullptr),
      codec(nullptr),
      av_frame(nullptr) {}

FFmpegRTSPStreamer::~FFmpegRTSPStreamer() { cleanup(); }

bool FFmpegRTSPStreamer::init()
{
  // av_register_all();
  avformat_network_init();
#if 1
  if (avformat_alloc_output_context2(&fmt_ctx, nullptr, "rtsp",
                                     rtsp_url.c_str()) < 0)
#else
  if (avformat_alloc_output_context2(&fmt_ctx, nullptr, "mpegts",
                                     rtsp_url.c_str()) < 0)
#endif
  {
    std::cerr << "Could not allocate output context." << std::endl;
    return false;
  }

  codec = avcodec_find_encoder(AV_CODEC_ID_H264);
  if (!codec)
  {
    std::cerr << "H264 codec not found." << std::endl;
    return false;
  }

  video_stream = avformat_new_stream(fmt_ctx, codec);
  if (!video_stream)
  {
    std::cerr << "Failed to create video stream." << std::endl;
    return false;
  }

  codec_ctx = avcodec_alloc_context3(codec);
  if (!codec_ctx)
  {
    std::cerr << "Failed to allocate codec context." << std::endl;
    return false;
  }

  codec_ctx->codec_id = codec->id;
  codec_ctx->codec_type = AVMEDIA_TYPE_VIDEO;
  codec_ctx->width = width;
  codec_ctx->height = height;
  codec_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
  // codec_ctx->pix_fmt = AV_PIX_FMT_YUYV422;
  codec_ctx->time_base = {1, fps};
  codec_ctx->gop_size = 12;

  codec_ctx->rc_buffer_size = 0;    // 清空编码缓冲区
  codec_ctx->rc_max_rate = 2000000; // 最大码率2Mbps
  codec_ctx->rc_min_rate = 1000000; // 最小码率1Mbps

  if (fmt_ctx->oformat->flags & AVFMT_GLOBALHEADER)
  {
    codec_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
  }

  if (avcodec_open2(codec_ctx, codec, nullptr) < 0)
  {
    std::cerr << "Failed to open codec." << std::endl;
    return false;
  }

  // 将编码器上下文的参数复制到流的编码参数中
  avcodec_parameters_from_context(video_stream->codecpar, codec_ctx);

  // rtsp不需要调用，调用会出错
  /*int ret = avio_open(&fmt_ctx->pb, rtsp_url.c_str(), AVIO_FLAG_WRITE);
  if (ret < 0) {
    char errbuf[AV_ERROR_MAX_STRING_SIZE];
    av_strerror(ret, errbuf, sizeof(errbuf));
    std::cerr << "Could not open RTSP stream: " << errbuf << std::endl;
    return false;
  }*/

  av_opt_set(fmt_ctx->priv_data, "rtsp_transport", "tcp", 0); // 使用TCP传输
  // av_opt_set(fmt_ctx->priv_data, "rtsp_transport", "udp", 0);
  av_opt_set(fmt_ctx->priv_data, "max_delay", "100000", 0); // 设置最大延迟为 100ms

  if (avformat_write_header(fmt_ctx, nullptr) < 0)
  {
    std::cerr << "Error occurred when writing header." << std::endl;
    return false;
  }

  av_frame = av_frame_alloc();
  av_frame->format = codec_ctx->pix_fmt;
  av_frame->width = width;
  av_frame->height = height;

  if (av_frame_get_buffer(av_frame, 32) < 0)
  {
    std::cerr << "Could not allocate frame buffer." << std::endl;
    return false;
  }

  sws_ctx_422_to_420 =
      sws_getContext(width, height, AV_PIX_FMT_YUYV422, // 源格式：YUV422
                     width, height, AV_PIX_FMT_YUV420P, // 目标格式：YUV420p
                     SWS_BICUBIC,                       // 缩放算法：可以选择其他算法，比如
                                                        // SWS_FAST_BILINEAR、SWS_BICUBIC 等
                     nullptr, nullptr, nullptr);

  if (!sws_ctx_422_to_420)
  {
    std::cerr << "Error initializing the conversion context sws_ctx_422_to_420."
              << std::endl;
    return false;
  }

  return true;
}

bool FFmpegRTSPStreamer::push_frame(const YUVFrame &frame)
{
#if DEBUG
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
#endif

  if (!av_frame || !frame.data)
  {
    std::cerr << "Invalid frame or uninitialized streamer." << std::endl;
    return false;
  }

  memcpy(av_frame->data[0], frame.data, width * height); // Y plane
  memcpy(av_frame->data[1], frame.data + width * height,
         width * height / 4); // U plane
  memcpy(av_frame->data[2], frame.data + width * height * 5 / 4,
         width * height / 4); // V plane

#if DEBUG
  end = std::chrono::high_resolution_clock::now();
  std::cout << "memcpy一帧耗时为:"
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                   .count()
            << "ms.\n";
#endif

  // 设置PTS，确保与时间基匹配
  av_frame->pts = av_rescale_q(frame_count++, (AVRational){1, fps},
                               video_stream->time_base);

  if (avcodec_send_frame(codec_ctx, av_frame) < 0)
  {
    std::cerr << "Failed to send frame to "
                 "encoder."
              << std::endl;
    return false;
  }

  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = nullptr;
  pkt.size = 0;

  if (avcodec_receive_packet(codec_ctx, &pkt) == 0)
  {
#if DEBUG
    end = std::chrono::high_resolution_clock::now();
    std::cout << "编码一帧耗时为:"
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                     .count()
              << "ms.\n";
#endif

    pkt.stream_index = video_stream->index;

    // 打印调试信息，检查PTS和DTS
    /*std::cout << "frame_count:" << frame_count << " PTS: " << pkt.pts
              << " DTS: " << pkt.dts << " size: " << pkt.size << std::endl;*/

    if (av_interleaved_write_frame(fmt_ctx, &pkt) < 0)
    {
      std::cerr << "Failed to write frame. PTS: " << pkt.pts
                << " DTS: " << pkt.dts << " size: " << pkt.size << std::endl;
      av_packet_unref(&pkt);
      return false;
    }
    av_packet_unref(&pkt);

#if DEBUG
    end = std::chrono::high_resolution_clock::now();
    std::cout << "发送一帧耗时为:"
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                     .count()
              << "ms.\n";
#endif
  }

#if DEBUG
  end = std::chrono::high_resolution_clock::now();
  std::cout << "frame_count:" << frame_count << ", 完整推送一帧耗时为:"
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                   .count()
            << "ms.\n";
#endif

  return true;
}

bool FFmpegRTSPStreamer::push_frame(unsigned char *frame)
{
#if DEBUG
  auto start = std::chrono::high_resolution_clock::now();
  auto end = std::chrono::high_resolution_clock::now();
#endif

  if (!av_frame || !frame)
  {
    std::cerr << "Invalid frame or uninitialized streamer." << std::endl;
    return false;
  }

  memcpy(av_frame->data[0], frame, width * height); // Y plane
  memcpy(av_frame->data[1], frame + width * height,
         width * height / 4); // U plane
  memcpy(av_frame->data[2], frame + width * height * 5 / 4,
         width * height / 4); // V plane

#if DEBUG
  end = std::chrono::high_resolution_clock::now();
  std::cout << "memcpy一帧耗时为:"
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                   .count()
            << "ms.\n";
#endif

  // 设置PTS，确保与时间基匹配
  av_frame->pts = av_rescale_q(frame_count++, (AVRational){1, fps},
                               video_stream->time_base);

  if (avcodec_send_frame(codec_ctx, av_frame) < 0)
  {
    std::cerr << "Failed to send frame to "
                 "encoder."
              << std::endl;
    return false;
  }

  AVPacket pkt;
  av_init_packet(&pkt);
  pkt.data = nullptr;
  pkt.size = 0;

  if (avcodec_receive_packet(codec_ctx, &pkt) == 0)
  {
#if DEBUG
    end = std::chrono::high_resolution_clock::now();
    std::cout << "编码一帧耗时为:"
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                     .count()
              << "ms.\n";
#endif

    pkt.stream_index = video_stream->index;

    // 打印调试信息，检查PTS和DTS
    /*std::cout << "frame_count:" << frame_count << " PTS: " << pkt.pts
              << " DTS: " << pkt.dts << " size: " << pkt.size << std::endl;*/

    if (av_interleaved_write_frame(fmt_ctx, &pkt) < 0)
    {
      std::cerr << "Failed to write frame. PTS: " << pkt.pts
                << " DTS: " << pkt.dts << " size: " << pkt.size << std::endl;
      av_packet_unref(&pkt);
      return false;
    }
    av_packet_unref(&pkt);

#if DEBUG
    end = std::chrono::high_resolution_clock::now();
    std::cout << "发送一帧耗时为:"
              << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                       start)
                     .count()
              << "ms.\n";
#endif
  }

#if DEBUG
  end = std::chrono::high_resolution_clock::now();
  std::cout << "frame_count:" << frame_count << ", 完整推送一帧耗时为:"
            << std::chrono::duration_cast<std::chrono::milliseconds>(end -
                                                                     start)
                   .count()
            << "ms.\n";
#endif

  return true;
}

void FFmpegRTSPStreamer::cleanup()
{
  if (fmt_ctx)
  {
    av_write_trailer(fmt_ctx);
    if (fmt_ctx->pb)
    {
      avio_close(fmt_ctx->pb);
    }
    avformat_free_context(fmt_ctx);
    fmt_ctx = nullptr;
  }

  if (codec_ctx)
  {
    avcodec_free_context(&codec_ctx);
    codec_ctx = nullptr;
  }

  if (av_frame)
  {
    av_frame_free(&av_frame);
    av_frame = nullptr;
  }

  if (sws_ctx_422_to_420)
  {
    sws_freeContext(sws_ctx_422_to_420);
    sws_ctx_422_to_420 = nullptr;
  }
}

void FFmpegRTSPStreamer::YUV422ToYUV420p(const unsigned char *yuv422,
                                         unsigned char *yuv420, int width,
                                         int height)
{
  int y_size = width * height;
  int uv_size = width * height / 4; // U 和 V 的大小

  // YUV422 数据的偏移量
  const unsigned char *y_data = yuv422;          // Y 数据起始位置
  const unsigned char *u_data = yuv422 + y_size; // U 数据起始位置
  const unsigned char *v_data =
      u_data + (width * height) / 2; // V 数据起始位置

  unsigned char *y_out = yuv420;          // Y 数据输出位置
  unsigned char *u_out = yuv420 + y_size; // U 数据输出位置
  unsigned char *v_out = u_out + uv_size; // V 数据输出位置

  // 复制 Y 数据（不做处理）
  memcpy(y_out, y_data, y_size);

  // 对 U 和 V 数据进行下采样（每 2 个像素计算一个 U 和一个 V）
  for (int i = 0; i < height / 2; i++)
  {
    for (int j = 0; j < width / 2; j++)
    {
      // 获取 YUV422 中相邻 2 个像素的 U 和 V 值
      int index422 = 2 * (i * width + j); // YUV422 中每 2 个像素对照的 U 和 V

      // 对应 YUV420p 中的 U 和 V 计算
      int index420_u = i * (width / 2) + j;
      int index420_v = index420_u;

      // 对 U 和 V 数据进行下采样
      u_out[index420_u] = (u_data[index422] + u_data[index422 + 1]) / 2;
      v_out[index420_v] = (v_data[index422] + v_data[index422 + 1]) / 2;
    }
  }
}

void FFmpegRTSPStreamer::YUV422ToYUV420pBySWS(const uint8_t *yuv422,
                                              uint8_t *yuv420, int width,
                                              int height)
{
  // 2. 设置源和目标图像缓冲区
  uint8_t *src_data[4] = {const_cast<uint8_t *>(yuv422), nullptr, nullptr,
                          nullptr};                       // YUV422 数据
  int src_linesize[4] = {width, width / 2, width / 2, 0}; // YUV422 的行间距

  uint8_t *dst_data[4] = {yuv420, nullptr, nullptr, nullptr}; // YUV420p 数据
  int dst_linesize[4] = {width, width / 2, width / 2, 0};     // YUV420p 的行间距

  // 3. 调用 sws_scale 进行格式转换
  int ret = sws_scale(sws_ctx_422_to_420, // 上下文
                      src_data,           // 源图像数据
                      src_linesize,       // 源图像行间距
                      0,                  // 源图像起始行
                      height,             // 图像高度
                      dst_data,           // 目标图像数据
                      dst_linesize        // 目标图像行间距
  );

  if (ret < 0)
  {
    std::cerr << "Error during conversion." << std::endl;
  }
}

void FFmpegRTSPStreamer::SaveYUV420pToFile(const uint8_t *yuv420p, int width,
                                           int height,
                                           const std::string &filename)
{
  // 创建并打开二进制文件
  std::ofstream file(filename, std::ios::binary);
  if (!file)
  {
    std::cerr << "无法打开文件：" << filename << std::endl;
    return;
  }

  // YUV420p 数据：Y平面 + U平面 + V平面
  int y_size = width * height; // Y平面大小
  int uv_size =
      width * height / 4; // U和V平面大小（每个平面是 Y 大小的四分之一）

  // 写入 Y、U、V 数据
  file.write(reinterpret_cast<const char *>(yuv420p), y_size); // Y平面
  file.write(reinterpret_cast<const char *>(yuv420p + y_size),
             uv_size); // U平面
  file.write(reinterpret_cast<const char *>(yuv420p + y_size + uv_size),
             uv_size); // V平面

  file.close();
  std::cout << "YUV 数据已保存为 " << filename << std::endl;
}

void FFmpegRTSPStreamer::SaveYUV422pToFile(const uint8_t *yuv422p, int width,
                                           int height,
                                           const std::string &filename)
{
  // 创建并打开二进制文件
  std::ofstream file(filename, std::ios::binary);
  if (!file)
  {
    std::cerr << "无法打开文件：" << filename << std::endl;
    return;
  }

  // YUV420p 数据：Y平面 + U平面 + V平面
  int y_size = width * height; // Y平面大小
  int uv_size = width * height / 2;

  // 写入 Y、U、V 数据
  file.write(reinterpret_cast<const char *>(yuv422p), y_size); // Y平面
  file.write(reinterpret_cast<const char *>(yuv422p + y_size),
             uv_size); // U平面
  file.write(reinterpret_cast<const char *>(yuv422p + y_size + uv_size),
             uv_size); // V平面

  file.close();
  std::cout << "YUV 数据已保存为 " << filename << std::endl;
}

void FFmpegRTSPStreamer::ConvertYUYVToYUV420P(const unsigned char *yuyv,
                                              unsigned char *yuv420p, int width,
                                              int height)
{
  int frameSize = width * height;

  // Pointers for Y, U, and V planes in YUV420P format
  unsigned char *yPlane = yuv420p;                     // Start of Y plane
  unsigned char *uPlane = yuv420p + frameSize;         // Start of U plane
  unsigned char *vPlane = yuv420p + frameSize * 5 / 4; // Start of V plane

  memset(yPlane, 0, frameSize);
  memset(uPlane, 0, frameSize / 4);
  memset(vPlane, 0, frameSize / 4);

  // Iterate through each pixel in the YUYV buffer
  for (int j = 0; j < height; j++)
  {
    for (int i = 0; i < width; i += 2)
    {
      // Calculate the index in the YUYV buffer
      int yuyvIndex = (j * width + i) * 2;

      // Extract Y, U, and V values
      unsigned char y1 = yuyv[yuyvIndex];     // Y1
      unsigned char u = yuyv[yuyvIndex + 1];  // U
      unsigned char y2 = yuyv[yuyvIndex + 2]; // Y2
      unsigned char v = yuyv[yuyvIndex + 3];  // V

      // Write Y values directly to the Y plane
      yPlane[j * width + i] = y1;
      yPlane[j * width + i + 1] = y2;

      // Write U and V values to their respective planes (downsampled)
      if (j % 2 == 0 && i % 2 == 0)
      {
        int uvIndex = (j / 2) * (width / 2) + (i / 2);
        uPlane[uvIndex] = u;
        vPlane[uvIndex] = v;
      }
    }
  }
}
