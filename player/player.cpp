#include <stdio.h>
#include "opencv2/opencv.hpp"

#include "VideoRenderer.h"

using namespace cv;

int main()
{
    VideoCapture cam;
    cam.open(0);
    if (!cam.isOpened())
    {
        printf("Can not open camera!\n");
        return -1;
    }
    Mat frame;
    cv::Mat rgbFrame;
    cam.read(frame);
    printf("camera frame size:%d,%d. channels: %d. type:%d.\n", frame.cols, frame.rows, frame.channels(), frame.type());

    VideoRenderer player(frame.cols, frame.rows);

    while (true)
    {
        cam.read(frame);
        cv::cvtColor(frame, rgbFrame, cv::COLOR_BGR2RGB);
        player.updateFrame(rgbFrame.data);
        player.render();
        if (waitKey(1) == '1')
        {
            break;
        }
    }
    return 0;
}