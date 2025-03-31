#include <iostream>
#include "RTSPStream.h"
#include "VideoRenderer.h"

void frame_loop();

std::unique_ptr<RTSPStream> stream;
// std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=0.sdp?";
std::string rtsp_url = "rtsp://192.168.147.128:8554/test";
bool frame_loop_rinning = false;
std::thread stream_thread_;

int main(int, char **)
{
    std::cout << "Hello, from receiver!\n";

    RTSPStream::player_type player_type = RTSPStream::player_type::opengl;

    switch (player_type)
    {
    case RTSPStream::player_type::none:
    {
        frame_loop_rinning = true;
        // stream_thread_ = std::thread(&frame_loop);
        frame_loop();
        break;
    }

    case RTSPStream::player_type::opengl:
    {
        stream = std::make_unique<RTSPStream>(rtsp_url, 640, 480, AV_PIX_FMT_RGB24);
        stream->startPlayer(RTSPStream::player_type::opengl);
        break;
    }

    case RTSPStream::player_type::opencv:
    {
        stream = std::make_unique<RTSPStream>(rtsp_url, 640, 480, AV_PIX_FMT_BGR24);
        stream->startPlayer(RTSPStream::player_type::opencv);
        break;
    }

    default:
        break;
    }

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    frame_loop_rinning = false;
    if (stream_thread_.joinable())
    {
        stream_thread_.join();
    }
    stream->stop();
}

void frame_loop()
{
    int width = 640;
    int height = 480;
    AVPixelFormat fmt = AV_PIX_FMT_RGB24;

    AVFrame *latest_frame = av_frame_alloc();
    latest_frame->width = width;
    latest_frame->height = height;
    latest_frame->format = fmt;

    // 分配 YUV420P 和 BGR24 格式的图像空间
    uint8_t *lates_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(fmt, width, height, 1));

    // 将数据绑定到 AVFrame
    av_image_fill_arrays(latest_frame->data, latest_frame->linesize, lates_buffer, fmt, width, height, 1);

    stream = std::make_unique<RTSPStream>(rtsp_url, width, height, fmt);
    stream->startPlayer(RTSPStream::player_type::opengl);

    std::unique_ptr<VideoRenderer> opengl_player;
    opengl_player = std::make_unique<VideoRenderer>(width, height);

    while (frame_loop_rinning)
    {
        stream->get_latest_frame(*latest_frame);
        opengl_player->updateFrame(latest_frame->data[0]);
        opengl_player->render();
    }

    av_frame_free(&latest_frame);
}
