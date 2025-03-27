#ifndef RKRGA_H
#define RKRGA_H

#pragma once

class RkRga
{
public:
    struct RgaImg
    {
        int width;
        int height;
        int format;
        unsigned char *data;
    };
    RkRga();
    ~RkRga();

    int rga_resize(RgaImg &in_img, RgaImg &out_img);
    void print_rga_info();

private:
    void printf_rga_img(const RgaImg &in_img);
};

#endif