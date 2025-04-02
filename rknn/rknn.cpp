#include "rknn.h"

#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"

#include <cstring>
#include <chrono>
#include <iostream>
#include <cmath>

#include "spdlog/spdlog.h"

Rknn::Rknn(const std::string &model_path, const std::string &model_labels_path, const uint obj_class_num, const uint box_prob_size)
    : _model_path(model_path), _model_labels_path(model_labels_path),
      _obj_class_num(obj_class_num), _prob_box_size(box_prob_size), _is_init(false)
{
}

Rknn::~Rknn()
{
    deinit();
}

int Rknn::init()
{
    int ret = 0;

    memset(&_rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    init_post_process(_obj_class_num, _model_labels_path.c_str());

    ret = init_yolov5_model(_model_path.c_str(), &_rknn_app_ctx);
    if (ret != 0)
    {
        // printf("init_yolov5_model fail! ret=%d model_path=%s\n", ret, _model_path.c_str());
        spdlog::error("init_yolov5_model fail! ret={} model_path={}", ret, _model_path.c_str());
        return ret;
    }

    _is_init = true;
    // printf("init_yolov5_model success! ret=%d model_path=%s\n", ret, _model_path.c_str());
    spdlog::info("init_yolov5_model success! ret={} model_path={}", ret, _model_path.c_str());
    return ret;
}

void Rknn::deinit()
{
    deinit_post_process(_obj_class_num);

    int ret = release_yolov5_model(&_rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_yolov5_model fail! ret=%d\n", ret);
        spdlog::error("release_yolov5_model fail! ret=={}", ret);
    }
    else
    {
        printf("release_yolov5_model success! ret=%d\n", ret);
        spdlog::info("release_yolov5_model success! ret=={}", ret);
    }
}

int Rknn::get_input_arrt(ModeAttr &attr)
{
    if (!_is_init)
        return -1;

    attr.width = _rknn_app_ctx.model_width;
    attr.height = _rknn_app_ctx.model_height;
    attr.channel = _rknn_app_ctx.model_channel;
    return 0;
}

void Rknn::set_detect_targets(const std::map<int, float> &detect_targets)
{
    _detect_targets = detect_targets;
}

int Rknn::inference(const Image &image, bool use_rga)
{
    // std::cout << "start_inference" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    int detected = 0;

    image_buffer_t src_image;
    src_image.width = image.width;
    src_image.height = image.height;
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.virt_addr = image.data;
    src_image.size = src_image.width * src_image.height * 3 * sizeof(unsigned char);

    object_detect_result_list od_results;

    int res = inference_yolov5_model(&_rknn_app_ctx, &src_image, &od_results, _obj_class_num, _prob_box_size, use_rga);
    if (res != 0)
    {
        std::cout << "inference_yolov5_model fail! res = " << res << std::endl;
        return -1;
    }

    /*auto inference = std::chrono::high_resolution_clock::now();
    auto inference_duration = std::chrono::duration_cast<std::chrono::milliseconds>(inference - start);
    std::cout << "inference time taken: " << inference_duration.count() << " milliseconds" << std::endl;*/

    // 画框和概率
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        /*printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(_obj_class_num, det_result->cls_id),
               det_result->box.left, det_result->box.top,
               det_result->box.right, det_result->box.bottom,
               det_result->prop);*/

        if (!_detect_targets.empty())
        {
            auto map_it = _detect_targets.cbegin();
            while (map_it != _detect_targets.cend())
            {
                if (map_it->first == det_result->cls_id && det_result->prop > map_it->second)
                {
                    detected += 1;
                    /*printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(_obj_class_num, det_result->cls_id),
                           det_result->box.left, det_result->box.top,
                           det_result->box.right, det_result->box.bottom,
                           det_result->prop);*/
                }
                ++map_it;
            }
        }

#if 1

        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);
        sprintf(text, "%s %.1f%%", coco_cls_to_name(_obj_class_num, det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);
#endif
    }

#if 1
    if (detected > 0)
    {
        spdlog::debug("save img");
        write_image("rknn_out.jpg", &src_image);
        // printf("*******************************************************************\n");
    }
#endif

#if 0
    // if (od_results.count > 0)
    {
        write_image("rknn_out.jpg", &src_image);
    }
#endif

    /*auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Total time taken: " << duration.count() << " milliseconds" << std::endl;
    printf("=================================================================\n");*/
    return detected;
}
