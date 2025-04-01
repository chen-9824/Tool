#include "RTSPStream.h"

#include <memory>
#include <opencv2/opencv.hpp>
#include <chrono>
#include <iomanip>

const size_t MAX_QUEUE_SIZE = 30; // 限制队列大小

const int MAX_DELAY_MS = 900; // 最大延迟时间，单位毫秒

using Clock = std::chrono::high_resolution_clock;
void print_elapsed_time(const Clock::time_point &start);

RTSPStream::RTSPStream(const std::string &url, int dst_width, int dst_height, AVPixelFormat dst_fmt)
    : url_(url), running_(false), _dst_width(dst_width), _dst_height(dst_height), _dst_fmt(dst_fmt)
{
}

RTSPStream::~RTSPStream()
{
    stop();
}

bool RTSPStream::startPlayer(player_type type)
{
    if (running_.load())
        return false;
    avformat_network_init();
    _player_type = type;
    running_.store(true);
    stream_thread_ = std::thread(&RTSPStream::streamLoop, this);
    return true;
}

void RTSPStream::stop()
{
    running_.store(false);
    if (stream_thread_.joinable())
    {
        stream_thread_.join();
    }
}

void RTSPStream::get_latest_frame(AVFrame &frame)
{
    if (!frame.data[0])
    {
        std::cerr << "Error: destination frame is not properly allocated!" << std::endl;
        return;
    }
    std::unique_lock<std::mutex> lock(frameQueueMutex);
    frameQueueCond.wait(lock, [this]()
                        { return latest_frame != nullptr; });
    av_frame_copy(&frame, latest_frame); // 如果需要降低获取最新帧数据的延迟，可以考虑减少一次复制，直接传回frame_bgr
    lock.unlock();
    std::cout << "get_latest_frame success!" << std::endl;
    print_frame_timestamp(latest_frame);
}

void RTSPStream::print_frame_timestamp(AVFrame *frame)
{
    if (!formatCtx || !frame)
    {
        std::cerr << "Invalid context or frame" << std::endl;
        return;
    }

    if (frame->pts != AV_NOPTS_VALUE)
    {
        // 将帧的 PTS 转换为秒
        AVRational timeBase = formatCtx->streams[videoStreamIndex]->time_base;
        double timestamp = frame->pts * av_q2d(timeBase);

        // 格式化输出为秒
        std::cout << "Frame PTS: " << frame->pts
                  << " | Time: " << timestamp << " seconds" << std::endl;

        auto ms = static_cast<int64_t>(timestamp * 1000);
        auto hours = (ms / (1000 * 60 * 60)) % 24;
        auto minutes = (ms / (1000 * 60)) % 60;
        auto seconds = (ms / 1000) % 60;
        auto milliseconds = ms % 1000;

        std::cout << "Time: " << hours << ":" << minutes << ":" << seconds << "." << milliseconds << std::endl;
    }
    else
    {
        std::cout << "No PTS available for this frame" << std::endl;
    }
}

int RTSPStream::ffmpeg_rtsp_init()
{
    // 3.设置打开媒体文件的相关参数
    AVDictionary *options = nullptr;
    av_dict_set(&options, "buffer_size", "6M", 0); // 设置 buffer_size 为 2MB
    // 以tcp方式打开,如果以udp方式打开将tcp替换为udp
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    // 设置超时断开连接时间,单位微秒,3000000表示3秒
    av_dict_set(&options, "stimeout", "1000000", 0);
    // 设置最大时延,单位微秒,1000000表示1秒
    av_dict_set(&options, "max_delay", "1000000", 0);
    // 自动开启线程数
    av_dict_set(&options, "threads", "auto", 0);
    // 设置分析持续时间
    // av_dict_set(&options, "analyzeduration", "1000000", 0);
    // 设置探测大小
    av_dict_set(&options, "probesize", "5000000", 0);

    if (avformat_open_input(&formatCtx, url_.c_str(), nullptr, &options) != 0)
    {
        std::cerr << "Failed to open RTSP stream: " << url_ << std::endl;
        return -1;
    }

    if (options != NULL)
    {
        av_dict_free(&options);
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
    {
        std::cerr << "Failed to find stream info." << std::endl;
        avformat_close_input(&formatCtx);
        return -1;
    }

    for (unsigned int i = 0; i < formatCtx->nb_streams; ++i)
    {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1)
    {
        std::cerr << "No video stream found." << std::endl;
        avformat_close_input(&formatCtx);
        return -1;
    }

    AVCodec *codec = avcodec_find_decoder(formatCtx->streams[videoStreamIndex]->codecpar->codec_id);
    // AVCodec *codec = avcodec_find_decoder_by_name("hevc_rkmpp");
    if (!codec)
    {
        std::cerr << "Unsupported codec." << std::endl;
        avformat_close_input(&formatCtx);
        return -1;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[videoStreamIndex]->codecpar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        std::cerr << "Failed to open codec." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }

    av_dump_format(formatCtx, videoStreamIndex, nullptr, 0);

    packet = av_packet_alloc();

    frame = av_frame_alloc();
    frame_bgr = av_frame_alloc();
    latest_frame = av_frame_alloc();

    frame->width = codecCtx->width;
    frame->height = codecCtx->height;
    frame->format = AV_PIX_FMT_YUV420P;

    frame_bgr->width = _dst_width;
    frame_bgr->height = _dst_height;
    frame_bgr->format = _dst_fmt;

    latest_frame->width = _dst_width;
    latest_frame->height = _dst_height;
    latest_frame->format = _dst_fmt;

    // 分配 YUV420P 和 BGR24 格式的图像空间
    int y_size = codecCtx->width * codecCtx->height;
    int uv_size = y_size / 4;
    uint8_t *yuv_buffer = (uint8_t *)av_malloc(y_size + 2 * uv_size);
    uint8_t *bgr_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(_dst_fmt, _dst_width, _dst_height, 1));
    uint8_t *lates_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(_dst_fmt, _dst_width, _dst_height, 1));

    // 将数据绑定到 AVFrame
    av_image_fill_arrays(frame->data, frame->linesize, yuv_buffer, AV_PIX_FMT_YUV420P, codecCtx->width, codecCtx->height, 1);
    av_image_fill_arrays(frame_bgr->data, frame_bgr->linesize, bgr_buffer, _dst_fmt, _dst_width, _dst_height, 1);
    av_image_fill_arrays(latest_frame->data, latest_frame->linesize, lates_buffer, _dst_fmt, _dst_width, _dst_height, 1);

    // 创建SwsContext用于格式转换
    swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, AV_PIX_FMT_YUV420P,
        _dst_width, _dst_height, _dst_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx)
    {
        std::cerr << "Failed to create SwsContext." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return -1;
    }
    return 0;
}

void RTSPStream::ffmpeg_rtsp_deinit()
{
    av_frame_free(&frame);
    av_frame_free(&frame_bgr);
    av_frame_free(&latest_frame);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
    sws_freeContext(swsCtx);
}

void RTSPStream::streamLoop()
{
    std::cout << "streamLoop start..." << std::endl;

    if (ffmpeg_rtsp_init() != 0)
    {
        std::cout << "ffmpeg_rtsp_init failed!" << std::endl;
        return;
    }

    auto start_time = Clock::now();

    startTime = av_gettime();

    while (running_ && av_read_frame(formatCtx, packet) >= 0)
    {
        if (is_pkt_outdated(packet))
        {
            // 丢弃过期帧
            av_packet_unref(packet);
            // avcodec_flush_buffers(codecCtx); // 清理解码器缓冲区，防止异常
            continue;
        }
        auto total_start_time = Clock::now();
        if (packet->stream_index == videoStreamIndex)
        {
            auto dec_start_time = Clock::now();
            if (avcodec_send_packet(codecCtx, packet) == 0)
            {
                while (avcodec_receive_frame(codecCtx, frame) == 0)
                {
                    std::cout << "解码耗时: " << std::endl;
                    print_elapsed_time(dec_start_time);
                    /*if (is_frame_outdated(frame))
                    {
                        // 丢弃过期帧
                        av_frame_unref(frame);
                        continue;
                    }*/

                    std::cout << "Frame received (" << frame->width << "x" << frame->height << ")" << " fmt: " << frame->format << std::endl;
                    //   将AVFrame转换为BGR格式
                    auto convert_start_time = Clock::now();
                    int ret = sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, frame_bgr->data, frame_bgr->linesize);
                    if (ret <= 0)
                    {
                        std::cerr << "sws_scale failed!" << std::endl;
                        continue; // 不显示无效图像
                    }
                    std::cout << "转换耗时: " << std::endl;
                    print_elapsed_time(convert_start_time);
                    std::cout << "Frame convert (" << frame_bgr->width << "x" << frame_bgr->height << ")" << " fmt: " << frame_bgr->format << std::endl;

                    if (frame_bgr->width > 0 && frame_bgr->height > 0)
                    {
                        if (_player_type == player_type::none)
                        {
                            std::lock_guard<std::mutex> lock(frameQueueMutex);
                            av_frame_copy(latest_frame, frame_bgr); // 如果需要降低获取最新帧数据的延迟，可以考虑减少一次复制，直接传回frame_bgr
                            latest_frame->pts = frame->pts;
                            // latest_frame->pkt_dts = frame->pkt_dts;
                            print_frame_timestamp(frame);
                            frameQueueCond.notify_one();
                        }
                        else if (_player_type == player_type::opencv)
                        {
                            print_frame_timestamp(frame);
                            print_elapsed_time(start_time);
                            std::cout << "=====================================" << std::endl;
                            /*cv::Mat img(frame_bgr->height, frame_bgr->width, CV_8UC3, frame_bgr->data[0], frame_bgr->linesize[0]);
                            cv::imshow("test", img);
                            cv::waitKey(1);*/
                        }
                        else if (_player_type == player_type::opengl)
                        {
                        }
                    }
                }
                std::cout << "=====================================" << std::endl;
            }
            av_packet_unref(packet);
        }
        /*std::cout << "全部耗时: " << std::endl;
        print_elapsed_time(total_start_time);
        std::cout << "**************************************************" << std::endl;*/

        // 延时(不然文件会立即全部播放完)
        // 定义时间基为微秒（AV_TIME_BASE）
        AVRational timeBase = {1, AV_TIME_BASE};
        // 将当前帧的时间戳（dts）从输入流的时间基转换为微秒时间基
        int64_t ptsTime = av_rescale_q(packet->dts, formatCtx->streams[videoStreamIndex]->time_base, timeBase);
        // 获取当前时间并减去播放开始时间，得到相对于播放开始时间的当前时间
        int64_t nowTime = av_gettime() - startTime;
        // 如果当前帧的显示时间还没到，就等待（睡眠）一段时间
        if (ptsTime > nowTime)
        {
            av_usleep(ptsTime - nowTime);
        }
    }
    ffmpeg_rtsp_deinit();
    running_.store(false);
}
bool RTSPStream::is_frame_outdated(AVFrame *frame)
{
    if (frame->pts == AV_NOPTS_VALUE)
    {
        std::cout << "No PTS available for this frame" << std::endl;
        return false;
    }
    AVRational timeBase = {1, AV_TIME_BASE};
    int64_t ptsTime = av_rescale_q(packet->dts, formatCtx->streams[videoStreamIndex]->time_base, timeBase);

    int64_t nowTime = av_gettime() - startTime;

    std::cout << "frame_time: " << ptsTime << ", nowTime:" << nowTime << std::endl;

    // 计算延迟
    int64_t delay_ms = (nowTime - ptsTime) / 1000;

    // 判断是否丢弃
    if (delay_ms > MAX_DELAY_MS)
    {
        std::cerr << "Frame too old: " << delay_ms << " ms → discarded" << std::endl;
        return true; // 丢弃过期帧
    }
    return false; // 正常处理
}
bool RTSPStream::is_pkt_outdated(AVPacket *packet)
{
    if (!packet || packet->size <= 0)
    {
        return false; // 无效包不丢弃
    }
    // 定义时间基为微秒（AV_TIME_BASE）
    AVRational timeBase = {1, AV_TIME_BASE};
    // 将当前帧的时间戳（dts）从输入流的时间基转换为微秒时间基
    int64_t ptsTime = av_rescale_q(packet->dts, formatCtx->streams[videoStreamIndex]->time_base, timeBase);
    // 获取当前时间并减去播放开始时间，得到相对于播放开始时间的当前时间
    int64_t nowTime = av_gettime() - startTime;
    // 如果当前帧的显示时间已过，抛弃
    int64_t delay_ms = (nowTime - ptsTime) / 1000;
    if (delay_ms > MAX_DELAY_MS)
    {
        // 检查是否是关键帧
        if ((packet->flags & AV_PKT_FLAG_KEY) == 0) // 非关键帧
        {
            std::cerr << "Frame too old: " << delay_ms << " ms → discarded" << std::endl;
            return true; // 丢弃过期帧
        }
    }
    return false; // 正常处理
}
void print_elapsed_time(const Clock::time_point &start)
{
    auto end = Clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    // 将毫秒转为时分秒
    auto hours = duration.count() / (1000 * 60 * 60);
    auto minutes = (duration.count() / (1000 * 60)) % 60;
    auto seconds = (duration.count() / 1000) % 60;
    auto milliseconds = duration.count() % 1000;

    std::cout << "Elapsed Time: "
              << std::setw(2) << std::setfill('0') << hours << ":"
              << std::setw(2) << std::setfill('0') << minutes << ":"
              << std::setw(2) << std::setfill('0') << seconds << "."
              << std::setw(3) << std::setfill('0') << milliseconds
              << std::endl;
}