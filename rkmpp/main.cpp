#include <iostream>

#include "RTSPStream.h"
#include "MppRtspRecv.h"
#include <opencv2/opencv.hpp>
#include <csignal>

#define OPENCV_SHOW 1

// std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=0.sdp?";
std::string rtsp_url = "rtsp://192.168.51.189:8554/test";

std::unique_ptr<RTSPStream> stream;
std::unique_ptr<MppRtspRecv> mpp_stream;
bool frame_loop_rinning = false;
std::thread stream_thread_;

void frame_loop();

void handle_signal(int signal)
{
    switch (signal)
    {
    case SIGINT:
    {
        frame_loop_rinning = false;
        break;
    }
    default:
    {
        break;
    }
    }
}

int main(int, char **)
{

    signal(SIGINT, handle_signal);
    std::cout << "Hello, from RKMPP!\n";

    frame_loop_rinning = true;
    frame_loop();

    return 0;
}

void frame_loop()
{

    int width = 640;
    int height = 640;
    AVPixelFormat fmt = AV_PIX_FMT_RGB24;

    AVFrame *latest_frame = av_frame_alloc();
    latest_frame->width = width;
    latest_frame->height = height;
    latest_frame->format = fmt;

    // 分配 YUV420P 和 BGR24 格式的图像空间
    uint8_t *lates_buffer = (uint8_t *)av_malloc(av_image_get_buffer_size(fmt, width, height, 1));

    // 将数据绑定到 AVFrame
    av_image_fill_arrays(latest_frame->data, latest_frame->linesize, lates_buffer, fmt, width, height, 1);

    // stream = std::make_unique<RTSPStream>(rtsp_url, width, height, fmt);
    // stream->startPlayer(RTSPStream::player_type::none);

    mpp_stream = std::make_unique<MppRtspRecv>(rtsp_url, width, height, fmt);
    mpp_stream->startPlayer(RTSPStream::player_type::none);

    while (frame_loop_rinning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

#if 0

    int dectect_num = 0;
    while (frame_loop_rinning)
    {
        // stream->get_latest_frame(*latest_frame);
        AVFrame *frame = stream->get_latest_frame();
        if (!frame)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
#if 0
        cv::Mat img(latest_frame->height, latest_frame->width, CV_8UC3, latest_frame->data[0], latest_frame->linesize[0]);
        cv::imshow("test", img);
        cv::waitKey(1);
#else

#if OPENCV_SHOW
        cv::Mat img(height, width, CV_8UC3, frame->data[0]);
        cv::Mat bgr_img;
        cv::cvtColor(img, bgr_img, cv::COLOR_BGR2RGB);
        cv::imshow("test", bgr_img);
        cv::waitKey(1);
#endif

#endif
    }
#endif

    av_frame_free(&latest_frame);
    mpp_stream->stop();
}