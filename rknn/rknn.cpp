#include "rknn.h"

#include "image_utils.h"
#include "file_utils.h"
#include "image_drawing.h"

#include <cstring>
#include <chrono>
#include <iostream>

Rknn::Rknn(const std::string &model_path, const std::string &model_labels_path, const uint obj_class_num, const uint box_prob_size)
    : _model_path(model_path), _model_labels_path(model_labels_path),
      _obj_class_num(obj_class_num), _prob_box_size(box_prob_size), _is_init(false)
{
}

int Rknn::init()
{
    int ret = 0;

    memset(&_rknn_app_ctx, 0, sizeof(rknn_app_context_t));

    init_post_process(_obj_class_num, _model_labels_path.c_str());

    ret = init_yolov5_model(_model_path.c_str(), &_rknn_app_ctx);
    if (ret != 0)
    {
        printf("init_yolov5_model fail! ret=%d model_path=%s\n", ret, _model_path.c_str());
        return ret;
    }

    _is_init = true;
    printf("init_yolov5_model success! ret=%d model_path=%s\n", ret, _model_path.c_str());
    return ret;
}

void Rknn::deinit()
{
    deinit_post_process(_obj_class_num);

    int ret = release_yolov5_model(&_rknn_app_ctx);
    if (ret != 0)
    {
        printf("release_yolov5_model fail! ret=%d\n", ret);
    }
    else
    {
        printf("release_yolov5_model success! ret=%d\n", ret);
    }
}

void Rknn::start_inference(const Image &image)
{
    std::cout << "start_inference" << std::endl;
    auto start = std::chrono::high_resolution_clock::now();

    image_buffer_t src_image;
    src_image.width = image.width;
    src_image.height = image.height;
    src_image.format = IMAGE_FORMAT_RGB888;
    src_image.virt_addr = image.data;
    src_image.size = src_image.width * src_image.height * 3 * sizeof(unsigned char);

    object_detect_result_list od_results;

    int res = inference_yolov5_model(&_rknn_app_ctx, &src_image, &od_results, _obj_class_num, _prob_box_size);
    if (res != 0)
    {
        std::cout << "inference_yolov5_model fail! res = " << res << std::endl;
        return;
    }

    // 画框和概率
    char text[256];
    for (int i = 0; i < od_results.count; i++)
    {
        object_detect_result *det_result = &(od_results.results[i]);
        printf("%s @ (%d %d %d %d) %.3f\n", coco_cls_to_name(_obj_class_num, det_result->cls_id),
               det_result->box.left, det_result->box.top,
               det_result->box.right, det_result->box.bottom,
               det_result->prop);
        int x1 = det_result->box.left;
        int y1 = det_result->box.top;
        int x2 = det_result->box.right;
        int y2 = det_result->box.bottom;

#if 1
        draw_rectangle(&src_image, x1, y1, x2 - x1, y2 - y1, COLOR_BLUE, 3);
        sprintf(text, "%s %.1f%%", coco_cls_to_name(_obj_class_num, det_result->cls_id), det_result->prop * 100);
        draw_text(&src_image, text, x1, y1 - 20, COLOR_RED, 10);
#endif
    }

#if 1
    if (res == 1)
    {
        write_image("test.jpg", &src_image);
    }
#endif

    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    std::cout << "Time taken: " << duration.count() << " milliseconds" << std::endl;
    printf("=================================================================\n");
}
