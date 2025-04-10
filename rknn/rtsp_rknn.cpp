#include <iostream>
#include <opencv2/opencv.hpp>
#include <csignal>

#include "RTSPStream.h"
#include "MyTcpClient.h"
#include "rknn.h"

#include "spdlog/spdlog.h"
#include <spdlog/sinks/basic_file_sink.h>

#include "INIReader.h"

#define OPENCV_SHOW 1

static void frame_loop();
static void start_store_client(std::string ip, int port);
static void reda_cfg();
static void handle_signal(int signal);

std::unique_ptr<RTSPStream> stream;
// std::string rtsp_url = "rtsp://192.168.51.168:5554/user=admin&password=&channel=1&stream=0.sdp?";
std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=1.sdp?";
// std::string rtsp_url = "rtsp://192.168.51.166:5554/user=admin&password=&channel=1&stream=0.sdp?";
//  std::string rtsp_url = "rtsp://admin:p@ssw0rd@192.168.63.65/h264/ch33/main/av_stream";
//   std::string rtsp_url = "rtsp://192.168.147.128:8554/test";
bool frame_loop_rinning = false;
std::thread stream_thread_;

std::atomic<bool> _store_start;
std::string store_cmd = "AIStore";
std::string store_server_ip = "127.0.0.1";
int store_server_port = 11223;

std::string model_path = "../model/rk3566/yolov5_3566.rknn";
std::string model_label_path = "../model/coco_80_labels_list.txt";
int obj_class_num = 80;
int max_detect_num = 1;
std::map<int, float> detect_targets = {
    {0, 0.7f}, // 人
    {2, 0.7f}, // 车
    {6, 0.7f},
    {7, 0.7f}};

int main()
{
    signal(SIGUSR1, handle_signal);
    signal(SIGINT, handle_signal);

    spdlog::set_level(spdlog::level::debug);
    reda_cfg();
#if 0
    auto logger = spdlog::basic_logger_mt("file_logger", "output.log");
    spdlog::set_default_logger(logger);
#endif

    frame_loop_rinning = true;
#if 1
    std::thread frame_loop_t(frame_loop);
    _store_start.store(false);
    std::thread store_t(start_store_client, store_server_ip, store_server_port);
#else
    stream = std::make_unique<RTSPStream>(rtsp_url, 640, 640, AV_PIX_FMT_BGR24);
    stream->startPlayer(RTSPStream::player_type::opencv);
#endif
    // spdlog::info("Press Enter to stop...");
    // std::cin.get();

    while (frame_loop_rinning)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }

    stream->stop();

    if (frame_loop_t.joinable())
        frame_loop_t.join();

    if (store_t.joinable())
        store_t.join();

    return 0;
}

void frame_loop()
{
    Rknn rknn(model_path, model_label_path, obj_class_num, obj_class_num + 5);
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
        int num = 0;
        // int num = rknn.inference(infer_img, 0);

        // num = rknn.inference(infer_img);

#if OPENCV_SHOW
        cv::Mat img(infer_img.height, infer_img.width, CV_8UC3, infer_img.data);
        cv::Mat bgr_img;
        cv::cvtColor(img, bgr_img, cv::COLOR_BGR2RGB);
        cv::imshow("test", bgr_img);
        cv::waitKey(1);
#endif

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

void reda_cfg()
{
    INIReader reader("../config.ini");

    if (reader.ParseError() < 0)
    {
        spdlog::error("Can't load config.ini: ../config.ini");
        return;
    }

    if (reader.HasSection("rknn"))
    {
        if (reader.HasValue("rknn", "model_path"))
            model_path = reader.GetString("rknn", "model_path", "");

        if (reader.HasValue("rknn", "model_label_path"))
            model_label_path = reader.GetString("rknn", "model_label_path", "");

        if (reader.HasValue("rknn", "obj_class_num"))
            obj_class_num = reader.GetInteger("rknn", "obj_class_num", 0);

        if (reader.HasValue("rknn", "max_detect_num"))
            max_detect_num = reader.GetInteger("rknn", "max_detect_num", 1);
    }

    if (reader.HasSection("rtsp"))
    {
        if (reader.HasValue("rtsp", "rtsp_url"))
            rtsp_url = reader.GetString("rtsp", "rtsp_url", "");
    }

    if (reader.HasSection("TcpClient"))
    {
        if (reader.HasValue("TcpClient", "store_server_ip"))
            store_server_ip = reader.GetString("TcpClient", "store_server_ip", "");

        if (reader.HasValue("TcpClient", "store_server_port"))
            store_server_port = reader.GetInteger("TcpClient", "store_server_port", 0);

        if (reader.HasValue("TcpClient", "store_cmd"))
            store_cmd = reader.GetString("TcpClient", "store_cmd", "");
    }

    spdlog::info("\n*********************AI模块******************************\n"
                 "RKNN: model_path: {}, model_label_path: {}, obj_class_num: {}, max_detect_num: {}\n"
                 "RTSP: rtsp_url: {}\n"
                 "TCP: store_server: {}:{}, store_cmd: {}\n"
                 "*************************************************************",
                 model_path, model_label_path, obj_class_num, max_detect_num, rtsp_url, store_server_ip, store_server_port, store_cmd);
}
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