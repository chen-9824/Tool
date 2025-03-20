#include <iostream>
#include <thread>
#include <chrono>
#include <string>
#include <cstdlib>

extern "C"
{
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libavutil/error.h>
#include <libswscale/swscale.h>
#include <libavdevice/avdevice.h>
#include <libavfilter/avfilter.h>
#include <libavfilter/buffersrc.h>
#include <libavfilter/buffersink.h>

#include <libavutil/avutil.h>
}

const char *cam_size = "1280x720";
const char *cam_fps = "9";
const char *cam_pixel_format = "yuyv422";
const char *input_device = "/dev/video0";

AVRational rtsp_time_base = {1, 9};
AVRational rtsp_fps = {9, 1};

const char *output_url = "rtsp://127.0.0.1:8554/test";

const char *filter_descr = "drawtext=fontfile=/home/cfan/Desktop/project/Tool/Tool/FfmpegRtspPusher/font/SourceHanSansCN-Normal.otf: "
                           "text='%{localtime}': x=w-tw-10: y=10: fontcolor=white: fontsize=24: box=1: boxcolor=black@0.5: rate=1";
AVFilterContext *buffersink_ctx;
AVFilterContext *buffersrc_ctx;
AVFilterGraph *filter_graph;

AVFormatContext *input_ctx = nullptr;
AVFormatContext *output_ctx = nullptr;
AVCodecContext *decoder_ctx = nullptr;
AVCodecContext *encoder_ctx = nullptr;
AVFrame *yuv420p_frame = nullptr;
AVFrame *yuyv_frame = nullptr;
AVFrame *yuv420p_filter_frame = nullptr;
SwsContext *sws_ctx = nullptr;
AVStream *out_stream = nullptr;
static int video_stream_index = -1;

static void
print_error(const std::string &msg, int errnum)
{
    char errbuf[128] = {0};
    av_strerror(errnum, errbuf, sizeof(errbuf));
    std::cerr << msg << ": " << errbuf << std::endl;
}

void free_all_res()
{
    // 释放所有资源
    if (input_ctx)
        avformat_close_input(&input_ctx);
    if (output_ctx)
        avformat_free_context(output_ctx);

    if (encoder_ctx)
        avcodec_free_context(&encoder_ctx);
    if (decoder_ctx)
        avcodec_free_context(&decoder_ctx);

    if (sws_ctx)
        sws_freeContext(sws_ctx);

    if (yuyv_frame)
        av_frame_free(&yuyv_frame);
    if (yuv420p_frame)
        av_frame_free(&yuv420p_frame);
    if (yuv420p_filter_frame)
        av_frame_free(&yuv420p_filter_frame);
}

static int init_input()
{
    int ret;
    // 1. 打开 USB 摄像头设备
    input_ctx = nullptr;
    AVDictionary *options = nullptr;

    AVInputFormat *ifmt = av_find_input_format("video4linux2");

    // 设置采集参数
    av_dict_set(&options, "framerate", cam_fps, 0);
    av_dict_set(&options, "video_size", cam_size, 0);
    av_dict_set(&options, "pixel_format", cam_pixel_format, 0);

    ret = avformat_open_input(&input_ctx, input_device, ifmt, &options);
    if (ret < 0)
    {
        print_error("无法打开 USB 摄像头", ret);
        return -1;
    }
    av_dict_free(&options);

    ret = avformat_find_stream_info(input_ctx, nullptr);
    if (ret < 0)
    {
        print_error("无法获取摄像头流信息", ret);
        free_all_res();
        return -1;
    }

    // 2. 查找视频流
    for (unsigned int i = 0; i < input_ctx->nb_streams; i++)
    {
        if (input_ctx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index < 0)
    {
        std::cerr << "未找到视频流" << std::endl;
        free_all_res();
        return -1;
    }

    av_dump_format(input_ctx, video_stream_index, input_device, 0);

    AVStream *in_video_stream = input_ctx->streams[video_stream_index];
    AVCodecParameters *codecpar = in_video_stream->codecpar;

    // 3. 查找解码器并打开解码器上下文
    AVCodec *decoder = avcodec_find_decoder(codecpar->codec_id);
    if (!decoder)
    {
        std::cerr << "未找到解码器" << std::endl;
        free_all_res();
        return -1;
    }
    decoder_ctx = avcodec_alloc_context3(decoder);
    if (!decoder_ctx)
    {
        std::cerr << "无法分配解码器上下文" << std::endl;
        free_all_res();
        return -1;
    }
    ret = avcodec_parameters_to_context(decoder_ctx, codecpar);
    if (ret < 0)
    {
        print_error("复制解码器参数失败", ret);
        free_all_res();
        return -1;
    }
    ret = avcodec_open2(decoder_ctx, decoder, nullptr);
    if (ret < 0)
    {
        print_error("打开解码器失败", ret);
        free_all_res();
        return -1;
    }

    return 0;
}

static int init_output()
{
    // 4. 创建 RTSP 输出上下文及编码器（H.264）
    int ret = 0;
    ret = avformat_alloc_output_context2(&output_ctx, nullptr, "rtsp", output_url);
    if (ret < 0 || !output_ctx)
    {
        print_error("创建输出上下文失败", ret);
        free_all_res();
        return -1;
    }

    // 查找 H.264 编码器
    AVCodec *encoder = avcodec_find_encoder(AV_CODEC_ID_H264);
    if (!encoder)
    {
        std::cerr << "未找到 H.264 编码器" << std::endl;
        free_all_res();
        return -1;
    }
    out_stream = avformat_new_stream(output_ctx, nullptr);
    if (!out_stream)
    {
        std::cerr << "创建输出流失败" << std::endl;
        free_all_res();
        return -1;
    }
    encoder_ctx = avcodec_alloc_context3(encoder);
    if (!encoder_ctx)
    {
        std::cerr << "无法分配编码器上下文" << std::endl;
        free_all_res();
        return -1;
    }
    // 设置编码参数与摄像头参数对应
    encoder_ctx->width = decoder_ctx->width;
    encoder_ctx->height = decoder_ctx->height;
    encoder_ctx->pix_fmt = AV_PIX_FMT_YUV420P;
    encoder_ctx->time_base = rtsp_time_base;
    encoder_ctx->framerate = rtsp_fps;

    encoder_ctx->bit_rate = 8000000; // 码率
    encoder_ctx->rc_buffer_size = 16000000;
    encoder_ctx->rc_max_rate = 8000000;
    encoder_ctx->rc_min_rate = 4000000;
    encoder_ctx->gop_size = 50;
    encoder_ctx->max_b_frames = 0; // 禁用 B 帧（降低延迟）
    encoder_ctx->qmin = 18;
    encoder_ctx->qmax = 23;
    av_opt_set(encoder_ctx->priv_data, "preset", "ultrafast", 0); // 编码速度优化
    av_opt_set(encoder_ctx->priv_data, "tune", "zerolatency", 0);
    av_opt_set(encoder_ctx->priv_data, "profile", "main", 0);
    av_opt_set(encoder_ctx->priv_data, "crf", "20", 0); // 质量控制参数
    if (output_ctx->oformat->flags & AVFMT_GLOBALHEADER)
        encoder_ctx->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    ret = avcodec_open2(encoder_ctx, encoder, nullptr);
    if (ret < 0)
    {
        print_error("打开编码器失败", ret);
        free_all_res();
        return -1;
    }
    ret = avcodec_parameters_from_context(out_stream->codecpar, encoder_ctx);
    if (ret < 0)
    {
        print_error("复制编码器参数到输出流失败", ret);
        free_all_res();
        return -1;
    }
    out_stream->time_base = encoder_ctx->time_base;

    // 打开 RTSP 输出 URL
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
    {
        ret = avio_open(&output_ctx->pb, output_url, AVIO_FLAG_WRITE);
        if (ret < 0)
        {
            print_error("打开输出 URL 失败", ret);
            free_all_res();
            return -1;
        }
    }
    ret = avformat_write_header(output_ctx, nullptr);
    if (ret < 0)
    {
        print_error("写入输出头失败", ret);
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
            avio_close(output_ctx->pb);
        free_all_res();
        return -1;
    }

    // 5. 创建像素格式转换上下文（若摄像头输出格式与编码器要求不一致）
    sws_ctx = sws_getContext(decoder_ctx->width, decoder_ctx->height, decoder_ctx->pix_fmt,
                             encoder_ctx->width, encoder_ctx->height, encoder_ctx->pix_fmt,
                             SWS_BICUBIC, nullptr, nullptr, nullptr);
    if (!sws_ctx)
    {
        std::cerr << "创建 SwsContext 失败" << std::endl;
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
            avio_close(output_ctx->pb);
        free_all_res();
        return -1;
    }
    // 为编码器分配一帧（转换后的数据）
    yuv420p_frame = av_frame_alloc();
    yuyv_frame = av_frame_alloc();
    yuv420p_filter_frame = av_frame_alloc();
    if (!yuv420p_frame || !yuyv_frame || !yuv420p_filter_frame)
    {
        std::cerr << "分配编码器帧失败" << std::endl;
        sws_freeContext(sws_ctx);
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
            avio_close(output_ctx->pb);
        free_all_res();
        return -1;
    }

    yuyv_frame->format = decoder_ctx->pix_fmt;
    yuyv_frame->width = decoder_ctx->width;
    yuyv_frame->height = decoder_ctx->height;
    av_frame_get_buffer(yuyv_frame, 32);

    yuv420p_filter_frame->format = encoder_ctx->pix_fmt;
    yuv420p_filter_frame->width = encoder_ctx->width;
    yuv420p_filter_frame->height = encoder_ctx->height;
    ret = av_image_alloc(yuv420p_filter_frame->data, yuv420p_filter_frame->linesize,
                         encoder_ctx->width, encoder_ctx->height,
                         encoder_ctx->pix_fmt, 32);

    yuv420p_frame->format = encoder_ctx->pix_fmt;
    yuv420p_frame->width = encoder_ctx->width;
    yuv420p_frame->height = encoder_ctx->height;
    ret = av_image_alloc(yuv420p_frame->data, yuv420p_frame->linesize,
                         encoder_ctx->width, encoder_ctx->height,
                         encoder_ctx->pix_fmt, 32);
    if (ret < 0)
    {
        print_error("分配编码器帧缓冲失败", ret);
        av_frame_free(&yuv420p_frame);
        sws_freeContext(sws_ctx);
        if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
            avio_close(output_ctx->pb);
        free_all_res();
        return -1;
    }

    return 0;
}

static int init_filters(const char *filters_descr)
{
    char args[512];
    int ret;
    const AVFilter *buffersrc = avfilter_get_by_name("buffer");
    const AVFilter *buffersink = avfilter_get_by_name("buffersink");
    AVFilterInOut *outputs = avfilter_inout_alloc();
    AVFilterInOut *inputs = avfilter_inout_alloc();
    enum AVPixelFormat pix_fmts[] = {AV_PIX_FMT_YUV420P, AV_PIX_FMT_NONE};
    AVBufferSinkParams *buffersink_params;

    filter_graph = avfilter_graph_alloc();

    /* buffer video source: the decoded frames from the decoder will be inserted here. */
    snprintf(args, sizeof(args),
             "video_size=%dx%d:pix_fmt=%d:time_base=%d/%d:pixel_aspect=%d/%d",
             encoder_ctx->width, encoder_ctx->height, encoder_ctx->pix_fmt,
             encoder_ctx->time_base.num, encoder_ctx->time_base.den,
             encoder_ctx->sample_aspect_ratio.num, encoder_ctx->sample_aspect_ratio.den);

    ret = avfilter_graph_create_filter(&buffersrc_ctx, buffersrc, "in",
                                       args, NULL, filter_graph);
    if (ret < 0)
    {
        printf("Cannot create buffer source\n");
        return ret;
    }

    /* buffer video sink: to terminate the filter chain. */
    buffersink_params = av_buffersink_params_alloc();
    buffersink_params->pixel_fmts = pix_fmts;
    ret = avfilter_graph_create_filter(&buffersink_ctx, buffersink, "out",
                                       NULL, buffersink_params, filter_graph);
    av_free(buffersink_params);
    if (ret < 0)
    {
        printf("Cannot create buffer sink\n");
        return ret;
    }

    /* Endpoints for the filter graph. */
    outputs->name = av_strdup("in");
    outputs->filter_ctx = buffersrc_ctx;
    outputs->pad_idx = 0;
    outputs->next = NULL;

    inputs->name = av_strdup("out");
    inputs->filter_ctx = buffersink_ctx;
    inputs->pad_idx = 0;
    inputs->next = NULL;

    if ((ret = avfilter_graph_parse_ptr(filter_graph, filters_descr,
                                        &inputs, &outputs, NULL)) < 0)
        return ret;

    if ((ret = avfilter_graph_config(filter_graph, NULL)) < 0)
        return ret;
    return 0;
}

int main()
{
    int ret = 0;
    // 初始化 FFmpeg 网络模块
    avdevice_register_all();
    avformat_network_init();

    if (init_input() != 0)
    {
        std::cerr << "init_input failed" << std::endl;
        return -1;
    }

    if (init_output() != 0)
    {
        std::cerr << "init_output failed" << std::endl;
        return -1;
    }

    if (init_filters(filter_descr) != 0)
    {
        std::cerr << "init_filters failed" << std::endl;
        return -1;
    }

    // 6. 读取摄像头数据，解码、转换、编码并推流
    AVPacket packet;
    av_init_packet(&packet);
    packet.data = nullptr;
    packet.size = 0;
    while ((ret = av_read_frame(input_ctx, &packet)) >= 0)
    {
        if (packet.stream_index == video_stream_index)
        {
            // 将 AVPacket 转为 AVFrame
            av_image_fill_arrays(
                yuyv_frame->data, yuyv_frame->linesize,
                packet.data, AV_PIX_FMT_YUYV422, decoder_ctx->width, decoder_ctx->height, 1);

            // 像素格式转换（转换到 YUV420P）
            sws_scale(sws_ctx, yuyv_frame->data, yuyv_frame->linesize, 0,
                      decoder_ctx->height, yuv420p_filter_frame->data, yuv420p_filter_frame->linesize);

            yuv420p_filter_frame->pts = packet.pts;

            if (av_buffersrc_add_frame(buffersrc_ctx, yuv420p_filter_frame) < 0)
            {
                printf("Error while add frame.\n");
                break;
            }

            /* pull filtered pictures from the filtergraph */
            ret = av_buffersink_get_frame(buffersink_ctx, yuv420p_frame);
            if (ret < 0)
                break;

            ret = avcodec_send_frame(encoder_ctx, yuv420p_frame);
            if (ret < 0)
            {
                print_error("发送帧到编码器失败", ret);
                break;
            }
            AVPacket enc_pkt;
            av_init_packet(&enc_pkt);
            enc_pkt.data = nullptr;
            enc_pkt.size = 0;
            while ((ret = avcodec_receive_packet(encoder_ctx, &enc_pkt)) >= 0)
            {
                enc_pkt.stream_index = out_stream->index;
                av_packet_rescale_ts(&enc_pkt, encoder_ctx->time_base, out_stream->time_base);
                ret = av_interleaved_write_frame(output_ctx, &enc_pkt);
                if (ret < 0)
                {
                    print_error("写入输出数据包失败", ret);
                    av_packet_unref(&enc_pkt);
                    break;
                }
                av_packet_unref(&enc_pkt);
            }
        }
        av_packet_unref(&packet);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }

    // 7. 刷新编码器，写入尾部信息
    ret = avcodec_send_frame(encoder_ctx, nullptr);
    if (ret < 0)
    {
        print_error("发送刷新帧到编码器失败", ret);
    }
    AVPacket enc_pkt;
    av_init_packet(&enc_pkt);
    enc_pkt.data = nullptr;
    enc_pkt.size = 0;
    while ((ret = avcodec_receive_packet(encoder_ctx, &enc_pkt)) >= 0)
    {
        enc_pkt.stream_index = out_stream->index;
        av_packet_rescale_ts(&enc_pkt, encoder_ctx->time_base, out_stream->time_base);
        ret = av_interleaved_write_frame(output_ctx, &enc_pkt);
        if (ret < 0)
        {
            print_error("写入刷新数据包失败", ret);
            av_packet_unref(&enc_pkt);
            break;
        }
        av_packet_unref(&enc_pkt);
    }

    ret = av_write_trailer(output_ctx);
    if (ret < 0)
    {
        print_error("写入尾部信息失败", ret);
    }
    if (!(output_ctx->oformat->flags & AVFMT_NOFILE))
        avio_close(output_ctx->pb);

    free_all_res();
    avformat_network_deinit();

    std::cout
        << "USB 摄像头推流结束" << std::endl;
    return 0;
}