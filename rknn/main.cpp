
#include "stb_image.h"
#include <cstring>
#include <iostream>
#include "rknn.h"
#include "RkRga.h"
#include "im2d.hpp"

std::string model_path = "../rknn/rknn_yolov5/model/rk3566/yolov5_3566.rknn";
std::string model_label_path = "../rknn/rknn_yolov5/model/coco_80_labels_list.txt";
int obj_class_num = 80;
uint box_prob_size = obj_class_num + 5;

std::string image_name = "../rknn/rknn_yolov5/model/bus.jpg";

int main()
{
    Rknn rknn(model_path, model_label_path, obj_class_num, box_prob_size);

    if (rknn.init() == 0)
    {
        int width, height, channels;
        unsigned char *img = stbi_load(image_name.c_str(), &width, &height, &channels, 0);
        if (!img)
        {
            std::cerr << "Failed to load image\n";
            return -1;
        }

        RkRga rga;

        RkRga::RgaImg in, out;
        in.width = width;
        in.height = height;
        in.data = img;
        in.format = RK_FORMAT_RGB_888;

        Rknn::ModeAttr attr;
        rknn.get_input_arrt(attr);
        out.width = attr.width;
        out.height = attr.height;
        out.data = nullptr;
        out.format = RK_FORMAT_RGB_888;

        Rknn::Image infer_img;
        infer_img.width = out.width;
        infer_img.height = out.height;
        /*if (rga.rga_resize(in, out) != IM_STATUS_SUCCESS)
        {
            return -1;
        }
        infer_img.data = out.data;
        */

        infer_img.data = in.data;
        printf("img width = %d, img height = %d\n", infer_img.width, infer_img.height);

        rknn.inference(infer_img);

        stbi_image_free(img);
        if (out.data)
            free(out.data);
    }

    return 0;
}
