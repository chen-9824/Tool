#include "RTSPStream.h"

#include "VideoRenderer.h"

#include <memory>
#include <opencv2/opencv.hpp>

std::unique_ptr<VideoRenderer> opengl_player;

const size_t MAX_QUEUE_SIZE = 30; // 限制队列大小
enum player_type
{
    none, // 不播放
    opencv,
    opengl
};
player_type _player_type = player_type::none;

RTSPStream::RTSPStream(const std::string &url) : url_(url),
                                                 running_(false)
{
}

RTSPStream::~RTSPStream()
{
    stop();
}

bool RTSPStream::start()
{
    if (running_.load())
        return false;
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

void RTSPStream::streamLoop()
{
    std::cout << "streamLoop start..." << std::endl;
    AVFormatContext *formatCtx = nullptr;
    AVCodecContext *codecCtx = nullptr;
    AVPacket *packet = nullptr;
    AVFrame *frame = nullptr;

    avformat_network_init();

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
        return;
    }

    if (options != NULL)
    {
        av_dict_free(&options);
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
    {
        std::cerr << "Failed to find stream info." << std::endl;
        avformat_close_input(&formatCtx);
        return;
    }

    int videoStreamIndex = -1;
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
        return;
    }

    AVCodec *codec = avcodec_find_decoder(formatCtx->streams[videoStreamIndex]->codecpar->codec_id);
    if (!codec)
    {
        std::cerr << "Unsupported codec." << std::endl;
        avformat_close_input(&formatCtx);
        return;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[videoStreamIndex]->codecpar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        std::cerr << "Failed to open codec." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    packet = av_packet_alloc();
    frame = av_frame_alloc();

    AVFrame *frame_bgr = av_frame_alloc();
    // 分配 YUV420P 和 BGR24 格式的图像空间
    int y_size = codecCtx->width * codecCtx->height;
    int uv_size = y_size / 4;
    uint8_t *yuv_buffer = (uint8_t *)av_malloc(y_size + 2 * uv_size);
    uint8_t *bgr_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(AV_PIX_FMT_BGR24, codecCtx->width, codecCtx->height, 1));

    // 将数据绑定到 AVFrame
    av_image_fill_arrays(frame->data, frame->linesize, yuv_buffer, AV_PIX_FMT_YUV420P, codecCtx->width, codecCtx->height, 1);
    av_image_fill_arrays(frame_bgr->data, frame_bgr->linesize, bgr_buffer, AV_PIX_FMT_BGR24, codecCtx->width, codecCtx->height, 1);

    // 创建SwsContext用于格式转换
    SwsContext *swsCtx = sws_getContext(
        codecCtx->width, codecCtx->height, AV_PIX_FMT_YUV420P,
        codecCtx->width, codecCtx->height, AV_PIX_FMT_BGR24,
        SWS_BILINEAR, nullptr, nullptr, nullptr);

    if (!swsCtx)
    {
        std::cerr << "Failed to create SwsContext." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return;
    }

    if (_player_type == player_type::opengl)
    {
        opengl_player = std::make_unique<VideoRenderer>(codecCtx->width, codecCtx->height);
    }
    cv::Mat rgbFrame;

    while (running_ && av_read_frame(formatCtx, packet) >= 0)
    {
        if (packet->stream_index == videoStreamIndex)
        {
            if (avcodec_send_packet(codecCtx, packet) == 0)
            {
                while (avcodec_receive_frame(codecCtx, frame) == 0)
                {
                    std::cout << "Frame received (" << frame->width << "x" << frame->height << ")" << " fmt: " << frame->format << std::endl;
                    // 将AVFrame转换为BGR格式
                    sws_scale(swsCtx, frame->data, frame->linesize, 0, frame->height, frame_bgr->data, frame_bgr->linesize);
                    cv::Mat img(frame->height, frame->width, CV_8UC3, frame_bgr->data[0], frame_bgr->linesize[0]);

                    if (_player_type == player_type::opencv)
                    {
                        cv::imshow("test", img);
                        cv::waitKey(1);
                    }
                    else if (_player_type == player_type::opengl)
                    {
                        cv::cvtColor(img, rgbFrame, cv::COLOR_BGR2RGB);
                        opengl_player->updateFrame(rgbFrame.data);
                        opengl_player->render();
                    }
                }
            }
        }
        av_packet_unref(packet);
    }

    av_frame_free(&frame);
    av_frame_free(&frame_bgr);
    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&formatCtx);
    sws_freeContext(swsCtx);
}
