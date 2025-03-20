#include <iostream>
#include "opencv2/opencv.hpp"

#include "FFmpegRTSPStreamer.h"

using namespace cv;

void store_file(const std::string &path, const cv::Mat &frame);
void store_yuv420p(const std::string &path, const cv::Mat &yuvFrame);

int main(int, char **)
{
    std::cout << "Hello, from push!\n";

    VideoCapture cam;
    cam.open(0);
    if (!cam.isOpened())
    {
        printf("Can not open camera!\n");
        return -1;
    }
    Mat frame;
    cv::Mat yuvFrame;
    cam.read(frame);
    printf("camera frame size:%d,%d. channels: %d. type:%d.\n", frame.cols, frame.rows, frame.channels(), frame.type());

    FFmpegRTSPStreamer streamer("rtsp://127.0.0.1:8554/test", frame.cols, frame.rows, 9);

    if (!streamer.init())
    {
        std::cerr << "Failed to initialize RTSP streamer." << std::endl;
        return -1;
    }

    streamer.add_time();

    while (true)
    {
        cam.read(frame);
        cv::cvtColor(frame, yuvFrame, cv::COLOR_BGR2YUV_I420);

        streamer.push_frame(yuvFrame.data);

        if (waitKey(1) == '1')
        {
            break;
        }
    }
    return 0;
}

// 追加写入
void store_file(const std::string &path, const cv::Mat &frame)
{
    // 确保数据尺寸正确
    std::vector<uint8_t> yuv_data(frame.total() * frame.elemSize());

    // 拷贝完整图像数据
    memcpy(yuv_data.data(), frame.data, yuv_data.size());

    // 以二进制方式追加写入
    std::ofstream outFile(path, std::ios::binary);
    if (!outFile)
    {
        std::cerr << "Failed to open file: " << path << std::endl;
        return;
    }
    outFile.write(reinterpret_cast<const char *>(yuv_data.data()), yuv_data.size());
    outFile.close();

    std::cout << "File saved successfully: " << path << std::endl;
}
