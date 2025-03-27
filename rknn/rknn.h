#ifndef RKNN_H
#define RKNN_H

#include "yolov5.h"

#include <string>
#include <atomic>
#include <mutex>

class Rknn
{
public:
    struct rknn_process_para
    {
        uint obj_class_num;
        uint box_prob_size;
        float nms_thresh;
        float box_thresh;
    };
    struct Size
    {
        uint width;
        uint height;
    };
    struct Point
    {
        long x;
        long y;
    };
    struct Image
    {
        uint width;
        uint height;
        unsigned char *data;
    };
    struct ModeAttr
    {
        uint width;
        uint height;
        uint channel;
    };

public:
    explicit Rknn(const std::string &model_path, const std::string &model_labels_path, const uint obj_class_num, const uint box_prob_size);

    int init();
    void deinit();

    int get_input_arrt(ModeAttr &attr);

    void inference(const Image &image);

    void start_inference_loop();
    void add_inference_img();

private:
    void inference_loop();

private:
    bool _is_init;

    rknn_app_context_t _rknn_app_ctx;
    std::string _model_path;
    std::string _model_labels_path;
    uint _obj_class_num;
    uint _prob_box_size;
};

#endif // RKNN_H
