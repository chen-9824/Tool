#ifndef RKNN_H
#define RKNN_H

#include "yolov5.h"

#include <string>
#include <atomic>
#include <mutex>
#include <map>
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
    struct InfereceRes
    {
        Point leftTop;
        Point rightBottom;
        int type;
        std::string label_name;
        float confidence = 0.0f; // 推理置信度

        int area() const
        {
            return (rightBottom.x - leftTop.x) * (rightBottom.y - leftTop.y);
        }

        Point center() const
        {
            return {
                (leftTop.x + rightBottom.x) / 2,
                (leftTop.y + rightBottom.y) / 2};
        }
    };

    enum Save_Img_Type
    {
        none = 0, // b不保存
        raw,      // 原图
        draw_rect // 添加识别框
    };

public:
    explicit Rknn(const std::string &model_path, const std::string &model_labels_path, const uint obj_class_num, const uint box_prob_size);
    ~Rknn();

    int init();
    void deinit();

    int get_input_arrt(ModeAttr &attr);

    // 设置检测对象id及置信度
    // 在推理得到的结果中判断是否存在待检测对象, 若存在且高于置信度, 推理函数返回 n, 表示推理结果存在 n 个待检测对象
    void set_detect_targets(const std::map<int, float> &detect_targets);
    // 返回值: -1 表示推理失败，0 表示推理成功且未检测到指定对象，其余值 n 表示推理结果存在 n 个待检测对象
    int inference(const Image &image, bool use_rga = true);

    // 返回值: -1 表示推理失败，0 表示推理成功
    // res: 存储推理得到的所有结果
    int inference(const Image &image, std::vector<InfereceRes> &rknn_res, bool use_rga = true);

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

    std::map<int, float> _detect_targets;
};

#endif // RKNN_H
