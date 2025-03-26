#ifndef RKNN_H
#define RKNN_H

#include "yolov5.h"

#include <string>

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

public:
    explicit Rknn(const std::string &model_path, const std::string &model_labels_path, const uint obj_class_num, const uint box_prob_size);

    int init();
    void deinit();

    void start_inference(const Image &image);

private:
    bool _is_init;

    rknn_app_context_t _rknn_app_ctx;
    std::string _model_path;
    std::string _model_labels_path;
    uint _obj_class_num;
    uint _prob_box_size;

    std::string storage_img_path;
};

#endif // RKNN_H
