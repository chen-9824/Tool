#include "RkRga.h"

#include "RgaUtils.h"
#include "im2d.hpp"
#include "utils.h"

#include <iostream>
#include <cstring>

RkRga::RkRga()
{
}

RkRga::~RkRga()
{
}

int RkRga::rga_resize(RgaImg &in_img, RgaImg &out_img)
{

    printf_rga_img(in_img);
    printf_rga_img(out_img);
    int ret = 0;
    int src_buf_size, dst_buf_size;

    rga_buffer_t src_img, dst_img;
    rga_buffer_handle_t src_handle, dst_handle;

    memset(&src_img, 0, sizeof(src_img));
    memset(&dst_img, 0, sizeof(dst_img));

    src_buf_size = in_img.width * in_img.height * get_bpp_from_format(in_img.format);
    dst_buf_size = out_img.width * out_img.height * get_bpp_from_format(out_img.format);

    out_img.data = (unsigned char *)malloc(dst_buf_size);
    memset(out_img.data, 0x80, dst_buf_size);

    src_handle = importbuffer_virtualaddr(in_img.data, src_buf_size);
    dst_handle = importbuffer_virtualaddr(out_img.data, dst_buf_size);
    if (src_handle == 0 || dst_handle == 0)
    {
        printf("importbuffer failed!\n");
        goto release_buffer;
    }

    src_img = wrapbuffer_handle(src_handle, in_img.width, in_img.height, in_img.format);
    dst_img = wrapbuffer_handle(dst_handle, out_img.width, out_img.height, out_img.format);

    /*
     * Scale up the src image to 1920*1080.
        --------------    ---------------------
        |            |    |                   |
        |  src_img   |    |     dst_img       |
        |            | => |                   |
        --------------    |                   |
                          |                   |
                          ---------------------
     */

    ret = imcheck(src_img, dst_img, {}, {});
    if (IM_STATUS_NOERROR != ret)
    {
        printf("%d, check error! %s", __LINE__, imStrError((IM_STATUS)ret));
        return -1;
    }

    ret = imresize(src_img, dst_img);
    if (ret == IM_STATUS_SUCCESS)
    {
        // printf("%s running success!\n", LOG_TAG);
        printf("running success!\n");
    }
    else
    {
        // printf("%s running failed, %s\n", LOG_TAG, imStrError((IM_STATUS)ret));
        printf(" running failed, %s\n", imStrError((IM_STATUS)ret));
        goto release_buffer;
    }

    // write_image_to_file(dst_buf, LOCAL_FILE_PATH, dst_width, dst_height, dst_format, 0);

release_buffer:
    if (src_handle)
        releasebuffer_handle(src_handle);
    if (dst_handle)
        releasebuffer_handle(dst_handle);

    return ret;
}

void RkRga::print_rga_info()
{
    std::cout << "rga info: \n"
              << querystring(RGA_ALL) << std::endl;
}
void RkRga::printf_rga_img(const RgaImg &in_img)
{
    std::cout << "img size: " << in_img.width << "*" << in_img.height << ", fmt: " << in_img.format << std::endl;
}