#include <iostream>
#include <opencv2/opencv.hpp>

#include "RTSPStream.h"

#include "rknn.h"

void frame_loop();

std::unique_ptr<RTSPStream> stream;
std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=0.sdp?";
// std::string rtsp_url = "rtsp://192.168.147.128:8554/test";
bool frame_loop_rinning = false;
std::thread stream_thread_;

std::string model_path = "../rknn/rknn_yolov5/model/rk3566/yolov5_3566.rknn";
std::string model_label_path = "../rknn/rknn_yolov5/model/coco_80_labels_list.txt";
int obj_class_num = 80;
uint box_prob_size = obj_class_num + 5;

std::string image_name = "../rknn/rknn_yolov5/model/bus.jpg";

int main()
{
    frame_loop_rinning = true;
    frame_loop();

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    frame_loop_rinning = false;
    stream->stop();
    return 0;
}

void frame_loop()
{
    Rknn rknn(model_path, model_label_path, obj_class_num, box_prob_size);
    rknn.init();

    Rknn::ModeAttr attr;
    rknn.get_input_arrt(attr);
    int width = attr.width;
    int height = attr.height;
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
    stream->startPlayer(RTSPStream::player_type::none);

    while (frame_loop_rinning)
    {
        stream->get_latest_frame(*latest_frame);
#if 0
        cv::Mat img(latest_frame->height, latest_frame->width, CV_8UC3, latest_frame->data[0], latest_frame->linesize[0]);
        cv::imshow("test", img);
        cv::waitKey(1);
#else
        Rknn::Image infer_img;
        infer_img.width = width;
        infer_img.height = height;
        infer_img.data = latest_frame->data[0];
        rknn.inference(infer_img, 0);
#endif
    }

    av_frame_free(&latest_frame);
}