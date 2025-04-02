#include <iostream>
#include <opencv2/opencv.hpp>

#include "RTSPStream.h"
#include "MyTcpClient.h"
#include "rknn.h"

#include "spdlog/spdlog.h"
#include <spdlog/sinks/basic_file_sink.h>

void frame_loop();
void start_store_client(std::string ip, int port);

std::unique_ptr<RTSPStream> stream;
std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=0.sdp?";
// std::string rtsp_url = "rtsp://admin:p@ssw0rd@192.168.63.65/h264/ch33/main/av_stream";
//  std::string rtsp_url = "rtsp://192.168.147.128:8554/test";
bool frame_loop_rinning = false;
std::thread stream_thread_;

std::atomic<bool> _store_start;
std::string store_cmd = "AIStore";
std::string store_server_ip = "127.0.0.1";
int store_server_port = 11223;

std::string model_path = "../rknn/rknn_yolov5/model/rk3566/yolov5_3566.rknn";
std::string model_label_path = "../rknn/rknn_yolov5/model/coco_80_labels_list.txt";
int obj_class_num = 80;
uint box_prob_size = obj_class_num + 5;
int max_detect_num = 1;
std::map<int, float> detect_targets = {
    {0, 0.7f}, // 人
    {2, 0.7f}, // 车
    {6, 0.7f},
    {7, 0.7f}};
/*std::map<int, float> detect_targets = {
    {58, 0.7f}};*/

std::string image_name = "../rknn/rknn_yolov5/model/bus.jpg";

int main()
{
    spdlog::set_level(spdlog::level::debug);
    auto logger = spdlog::basic_logger_mt("file_logger", "output.log");
    spdlog::set_default_logger(logger);

    frame_loop_rinning = true;
#if 0
    std::thread frame_loop_t(frame_loop);
    _store_start.store(false);
    std::thread store_t(start_store_client, store_server_ip, store_server_port);
#else
    stream = std::make_unique<RTSPStream>(rtsp_url, 640, 640, AV_PIX_FMT_RGB24);
    stream->startPlayer(RTSPStream::player_type::opencv);
#endif
    spdlog::info("Press Enter to stop...");
    std::cin.get();

    frame_loop_rinning = false;
    stream->stop();
    return 0;
}

void frame_loop()
{
    Rknn rknn(model_path, model_label_path, obj_class_num, box_prob_size);
    rknn.init();
    rknn.set_detect_targets(detect_targets);

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
        Rknn::Image infer_img;
        infer_img.width = width;
        infer_img.height = height;
        infer_img.data = frame->data[0];
        int num = rknn.inference(infer_img, 0);
        if (num > 0)
        {
            spdlog::debug("inference get target:  {}, dectect_num: {}", num, dectect_num);
            dectect_num += 1;
            if (dectect_num > max_detect_num)
            {
                _store_start.store(true);
                dectect_num = 0;
            }
        }
        else
        {
            dectect_num = 0;
        }
#endif
    }

    av_frame_free(&latest_frame);
}

void start_store_client(std::string ip, int port)
{

    std::unique_ptr<MyTcpClient> _client =
        std::make_unique<MyTcpClient>(ip, port);
    _client->connectToServer();

    while (frame_loop_rinning)
    {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (_store_start)
        {

            spdlog::debug("store_client sendMessage: {}", store_cmd);
            _client->sendMessage(store_cmd);
            _store_start.store(false);
        }
    }
}