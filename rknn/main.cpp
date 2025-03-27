#include <iostream>
#include "rknn.h"
#include "opencv2/opencv.hpp"

std::string image_name = "./test.jpg";

int main()
{
    std::cout << "Hello, AArch64!" << std::endl;
    Rknn rknn("../rknn/rknn_yolov5/model/rk3566/yolov5_3566.rknn", "../rknn/rknn_yolov5/model/coco_80_labels_list.txt", 80, 80 + 5);

    cv::Mat rgbFrame;

    if (rknn.init() == 0)
    {
        cv::Mat orig_img = cv::imread(image_name, cv::IMREAD_COLOR);
        if (orig_img.empty())
        {
            printf("cv::imread %s fail!\n", image_name.c_str());
            return -1;
        }
        cv::cvtColor(orig_img, rgbFrame, cv::COLOR_BGR2RGB);
        Rknn::Image infer_img;
        infer_img.width = rgbFrame.cols;
        infer_img.height = rgbFrame.rows;
        infer_img.data = rgbFrame.data;
        printf("img width = %d, img height = %d\n", infer_img.width, infer_img.height);

        rknn.start_inference(infer_img);
    }

    return 0;
}