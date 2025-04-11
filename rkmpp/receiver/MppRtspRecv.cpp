#include "MppRtspRecv.h"
#include "spdlog/spdlog.h"
#include <opencv2/opencv.hpp>

typedef struct DecBufMgrImpl_t
{
    MppDecBufMode buf_mode;
    RK_U32 buf_count;
    RK_U32 buf_size;
    MppBufferGroup group;
    MppBuffer *bufs;
} DecBufMgrImpl;

MPP_RET dec_buf_mgr_init(DecBufMgr *mgr)
{
    DecBufMgrImpl *impl = NULL;
    MPP_RET ret = MPP_NOK;

    if (mgr)
    {
        impl = mpp_calloc(DecBufMgrImpl, 1);
        if (impl)
        {
            ret = MPP_OK;
        }
        else
        {
            mpp_err_f("failed to create decoder buffer manager\n");
        }

        *mgr = impl;
    }

    return ret;
}

void dec_buf_mgr_deinit(DecBufMgr mgr)
{
    DecBufMgrImpl *impl = (DecBufMgrImpl *)mgr;

    if (NULL == impl)
        return;

    /* release buffer group for half internal and external mode */
    if (impl->group)
    {
        mpp_buffer_group_put(impl->group);
        impl->group = NULL;
    }

    /* release the buffers used in external mode */
    if (impl->buf_count && impl->bufs)
    {
        RK_U32 i;

        for (i = 0; i < impl->buf_count; i++)
        {
            if (impl->bufs[i])
            {
                mpp_buffer_put(impl->bufs[i]);
                impl->bufs[i] = NULL;
            }
        }

        MPP_FREE(impl->bufs);
    }

    MPP_FREE(impl);
}

MppBufferGroup dec_buf_mgr_setup(DecBufMgr mgr, RK_U32 size, RK_U32 count, MppDecBufMode mode)
{
    DecBufMgrImpl *impl = (DecBufMgrImpl *)mgr;
    MPP_RET ret = MPP_NOK;

    if (!impl)
        return NULL;

    /* cleanup old buffers if previous buffer group exists */
    if (impl->group)
    {
        if (mode != impl->buf_mode)
        {
            /* switch to different buffer mode just release old buffer group */
            mpp_buffer_group_put(impl->group);
            impl->group = NULL;
        }
        else
        {
            /* otherwise just cleanup old buffers */
            mpp_buffer_group_clear(impl->group);
        }

        /* if there are external mode old buffers do cleanup */
        if (impl->bufs)
        {
            RK_U32 i;

            for (i = 0; i < impl->buf_count; i++)
            {
                if (impl->bufs[i])
                {
                    mpp_buffer_put(impl->bufs[i]);
                    impl->bufs[i] = NULL;
                }
            }

            MPP_FREE(impl->bufs);
        }
    }

    switch (mode)
    {
    case MPP_DEC_BUF_HALF_INT:
    {
        /* reuse previous half internal buffer group and just reconfig limit */
        if (NULL == impl->group)
        {
            ret = mpp_buffer_group_get_internal(&impl->group, MPP_BUFFER_TYPE_ION);
            if (ret)
            {
                mpp_err_f("get mpp internal buffer group failed ret %d\n", ret);
                break;
            }
        }
        /* Use limit config to limit buffer count and buffer size */
        ret = mpp_buffer_group_limit_config(impl->group, size, count);
        if (ret)
        {
            mpp_err_f("limit buffer group failed ret %d\n", ret);
        }
    }
    break;
    case MPP_DEC_BUF_INTERNAL:
    {
        /* do nothing juse keep buffer group empty */
        mpp_assert(NULL == impl->group);
        ret = MPP_OK;
    }
    break;
    case MPP_DEC_BUF_EXTERNAL:
    {
        RK_U32 i;
        MppBufferInfo commit;

        impl->bufs = mpp_calloc(MppBuffer, count);
        if (!impl->bufs)
        {
            mpp_err_f("create %d external buffer record failed\n", count);
            break;
        }

        /* reuse previous external buffer group */
        if (NULL == impl->group)
        {
            ret = mpp_buffer_group_get_external(&impl->group, MPP_BUFFER_TYPE_ION);
            if (ret)
            {
                mpp_err_f("get mpp external buffer group failed ret %d\n", ret);
                break;
            }
        }

        /*
         * NOTE: Use default misc allocater here as external allocator for demo.
         * But in practical case the external buffer could be GraphicBuffer or gst dmabuf.
         * The misc allocator will cause the print at the end like:
         * ~MppBufferService cleaning misc group
         */
        commit.type = MPP_BUFFER_TYPE_ION;
        commit.size = size;

        for (i = 0; i < count; i++)
        {
            ret = mpp_buffer_get(NULL, &impl->bufs[i], size);
            if (ret || NULL == impl->bufs[i])
            {
                mpp_err_f("get misc buffer failed ret %d\n", ret);
                break;
            }

            commit.index = i;
            commit.ptr = mpp_buffer_get_ptr(impl->bufs[i]);
            commit.fd = mpp_buffer_get_fd(impl->bufs[i]);

            ret = mpp_buffer_commit(impl->group, &commit);
            if (ret)
            {
                mpp_err_f("external buffer commit failed ret %d\n", ret);
                break;
            }
        }
    }
    break;
    default:
    {
        mpp_err_f("unsupport buffer mode %d\n", mode);
    }
    break;
    }

    if (ret)
    {
        dec_buf_mgr_deinit(impl);
        impl = NULL;
    }
    else
    {
        impl->buf_count = count;
        impl->buf_size = size;
        impl->buf_mode = mode;
    }

    return impl ? impl->group : NULL;
}

static void rearrange_pix(RK_U8 *tmp_line, RK_U8 *base, RK_U32 n)
{
    RK_U16 *pix = (RK_U16 *)(tmp_line + n * 16);
    RK_U16 *base_u16 = (RK_U16 *)(base + n * 10);

    pix[0] = base_u16[0] & 0x03FF;
    pix[1] = (base_u16[0] & 0xFC00) >> 10 | (base_u16[1] & 0x000F) << 6;
    pix[2] = (base_u16[1] & 0x3FF0) >> 4;
    pix[3] = (base_u16[1] & 0xC000) >> 14 | (base_u16[2] & 0x00FF) << 2;
    pix[4] = (base_u16[2] & 0xFF00) >> 8 | (base_u16[3] & 0x0003) << 8;
    pix[5] = (base_u16[3] & 0x0FFC) >> 2;
    pix[6] = (base_u16[3] & 0xF000) >> 12 | (base_u16[4] & 0x003F) << 4;
    pix[7] = (base_u16[4] & 0xFFC0) >> 6;
}

void dump_mpp_frame_to_file(MppFrame frame, FILE *fp)
{
    RK_U32 width = 0;
    RK_U32 height = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;
    MppFrameFormat fmt = MPP_FMT_YUV420SP;
    MppBuffer buffer = NULL;
    RK_U8 *base = NULL;

    if (NULL == fp || NULL == frame)
        return;

    width = mpp_frame_get_width(frame);
    height = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    fmt = mpp_frame_get_fmt(frame);
    buffer = mpp_frame_get_buffer(frame);

    if (NULL == buffer)
        return;

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);

    if (MPP_FRAME_FMT_IS_RGB(fmt) && MPP_FRAME_FMT_IS_LE(fmt))
    {
        // fmt &= MPP_FRAME_FMT_MASK;
        fmt = (MppFrameFormat)(fmt & MPP_FRAME_FMT_MASK);
    }
    switch (fmt & MPP_FRAME_FMT_MASK)
    {
    case MPP_FMT_YUV422SP:
    {
        /* YUV422SP -> YUV422P for better display */
        RK_U32 i, j;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;
        RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height * 2);
        RK_U8 *tmp_u = tmp;
        RK_U8 *tmp_v = tmp + width * height / 2;

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);

        for (i = 0; i < height; i++, base_c += h_stride)
        {
            for (j = 0; j < width / 2; j++)
            {
                tmp_u[j] = base_c[2 * j + 0];
                tmp_v[j] = base_c[2 * j + 1];
            }
            tmp_u += width / 2;
            tmp_v += width / 2;
        }

        fwrite(tmp, 1, width * height, fp);
        mpp_free(tmp);
    }
    break;
    case MPP_FMT_YUV420SP_VU:
    case MPP_FMT_YUV420SP:
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;

        for (i = 0; i < height; i++, base_y += h_stride)
        {
            fwrite(base_y, 1, width, fp);
        }
        for (i = 0; i < height / 2; i++, base_c += h_stride)
        {
            fwrite(base_c, 1, width, fp);
        }
    }
    break;
    case MPP_FMT_YUV420P:
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;

        for (i = 0; i < height; i++, base_y += h_stride)
        {
            fwrite(base_y, 1, width, fp);
        }
        for (i = 0; i < height / 2; i++, base_c += h_stride / 2)
        {
            fwrite(base_c, 1, width / 2, fp);
        }
        for (i = 0; i < height / 2; i++, base_c += h_stride / 2)
        {
            fwrite(base_c, 1, width / 2, fp);
        }
    }
    break;
    case MPP_FMT_YUV420SP_10BIT:
    {
        RK_U32 i, k;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;
        RK_U8 *tmp_line = (RK_U8 *)mpp_malloc(RK_U16, width);

        if (!tmp_line)
        {
            mpp_log("tmp_line malloc fail");
            return;
        }

        for (i = 0; i < height; i++, base_y += h_stride)
        {
            for (k = 0; k < MPP_ALIGN(width, 8) / 8; k++)
                rearrange_pix(tmp_line, base_y, k);
            fwrite(tmp_line, width * sizeof(RK_U16), 1, fp);
        }

        for (i = 0; i < height / 2; i++, base_c += h_stride)
        {
            for (k = 0; k < MPP_ALIGN(width, 8) / 8; k++)
                rearrange_pix(tmp_line, base_c, k);
            fwrite(tmp_line, width * sizeof(RK_U16), 1, fp);
        }

        MPP_FREE(tmp_line);
    }
    break;
    case MPP_FMT_YUV444SP:
    {
        /* YUV444SP -> YUV444P for better display */
        RK_U32 i, j;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;
        RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height * 2);
        RK_U8 *tmp_u = tmp;
        RK_U8 *tmp_v = tmp + width * height;

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);

        for (i = 0; i < height; i++, base_c += h_stride * 2)
        {
            for (j = 0; j < width; j++)
            {
                tmp_u[j] = base_c[2 * j + 0];
                tmp_v[j] = base_c[2 * j + 1];
            }
            tmp_u += width;
            tmp_v += width;
        }

        fwrite(tmp, 1, width * height * 2, fp);
        mpp_free(tmp);
    }
    break;
    case MPP_FMT_YUV444SP_10BIT:
    {
        RK_U32 i, k;
        RK_U8 *base_y = base;
        RK_U8 *base_c = base + h_stride * v_stride;
        RK_U8 *tmp_line = (RK_U8 *)mpp_malloc(RK_U16, width);

        if (!tmp_line)
        {
            mpp_log("tmp_line malloc fail");
            return;
        }

        for (i = 0; i < height; i++, base_y += h_stride)
        {
            for (k = 0; k < MPP_ALIGN(width, 8) / 8; k++)
                rearrange_pix(tmp_line, base_y, k);
            fwrite(tmp_line, width * sizeof(RK_U16), 1, fp);
        }

        for (i = 0; i < (height * 2); i++, base_c += h_stride)
        {
            for (k = 0; k < MPP_ALIGN(width, 8) / 8; k++)
                rearrange_pix(tmp_line, base_c, k);
            fwrite(tmp_line, width * sizeof(RK_U16), 1, fp);
        }

        MPP_FREE(tmp_line);
    }
    break;
    case MPP_FMT_YUV400:
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *tmp = mpp_malloc(RK_U8, h_stride * height);

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);

        mpp_free(tmp);
    }
    break;
    case MPP_FMT_ARGB8888:
    case MPP_FMT_ABGR8888:
    case MPP_FMT_BGRA8888:
    case MPP_FMT_RGBA8888:
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *tmp = mpp_malloc(RK_U8, width * height * 4);

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width * 4, fp);

        mpp_free(tmp);
    }
    break;
    case MPP_FMT_YUV422_YUYV:
    case MPP_FMT_YUV422_YVYU:
    case MPP_FMT_YUV422_UYVY:
    case MPP_FMT_YUV422_VYUY:
    case MPP_FMT_RGB565:
    case MPP_FMT_BGR565:
    case MPP_FMT_RGB555:
    case MPP_FMT_BGR555:
    case MPP_FMT_RGB444:
    case MPP_FMT_BGR444:
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *tmp = mpp_malloc(RK_U8, width * height * 2);

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width * 2, fp);

        mpp_free(tmp);
    }
    break;
    case MPP_FMT_RGB888:
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *tmp = mpp_malloc(RK_U8, width * height * 3);

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width * 3, fp);

        mpp_free(tmp);
    }
    break;
    default:
    {
        mpp_err("not supported format %d\n", fmt);
    }
    break;
    }
}

MppRtspRecv::MppRtspRecv(const std::string &url, int dst_width, int dst_height, AVPixelFormat dst_fmt)
    : RTSPStream(url, dst_width, dst_height, dst_fmt)
{
}

MppRtspRecv::~MppRtspRecv()
{
    rtsp_deinit();
}

bool MppRtspRecv::startPlayer(player_type type)
{
    if (running_.load())
        return false;
    avformat_network_init();
    running_.store(true);
    t_s = mpp_time();
    stream_thread_ = std::thread(&MppRtspRecv::streamLoopWithMpp, this);
    return true;
}

void MppRtspRecv::stop()
{
    spdlog::info("RTSPStream stoping...");
    running_.store(false);
    // frameQueueCond.notify_all();
    if (stream_thread_.joinable())
    {
        stream_thread_.join();
    }
    t_e = mpp_time();
    RK_S64 elapsed_time = t_e - t_s;

    float frame_rate = (float)frame_count * 1000000 / elapsed_time;
    RK_S64 delay = first_frm - first_pkt;

    mpp_log("decode %d frames time %lld ms delay %3d ms fps %3.2f\n", frame_count,
            (RK_S64)(elapsed_time / 1000), (RK_S32)(delay / 1000), frame_rate);

    mpp_log("test success max memory %.2f MB\n", max_usage / (float)(1 << 20));

    spdlog::info("RTSPStream stop!");
}

void MppRtspRecv::streamLoopWithMpp()
{
    bool rtsp_initialized = false;
    int frameFinish;
    int error_frame_count = 0;
    startTime = av_gettime();
    while (running_)
    {
        if (!rtsp_initialized)
        {
            rtsp_initialized = rtsp_init();

            if (!rtsp_initialized)
            {
                // std::cout << "ffmpeg_rtsp_init failed, " << std::endl;
                spdlog::error("ffmpeg_rtsp_init failed, wait reconncet...");
                std::this_thread::sleep_for(std::chrono::seconds(5));
                continue;
            }
        }

        try
        {
            frameFinish = av_read_frame(formatCtx, packet);
            if (frameFinish >= 0)
            {
                if (packet->stream_index == videoStreamIndex)
                {
                    error_frame_count = 0;

                    // 3.2.2 配置输入pkt
                    mpp_packet_set_data(mpp_packet, packet->data);
                    mpp_packet_set_size(mpp_packet, packet->size);
                    mpp_packet_set_pos(mpp_packet, packet->data);
                    mpp_packet_set_length(mpp_packet, packet->size);

                    RK_U32 pkt_done = 0;
                    int ret = 0;

                    do
                    {
                        RK_S32 dec_frame_timeout = 30;
                        // RK_U32 frm_eos = 1;

                        if (!pkt_done)
                        {
                            // 3.2.3 packet传入解码器
                            ret = mpi->decode_put_packet(mpp_ctx, mpp_packet);
                            if (ret == MPP_OK)
                            {
                                pkt_done = 1;
                                if (!first_pkt)
                                {
                                    first_pkt = mpp_time(); // 记录第一个成功送入解码器的时间戳
                                }
                            }
                        }

                        do
                        {
                            RK_S32 get_frm = 0;
                            MppFrame mpp_frame = NULL;
                        try_again:
                            // 3.2.4 从解码器获取解码后的帧
                            ret = mpi->decode_get_frame(mpp_ctx, &mpp_frame);
                            // 超时等待
                            if (ret == MPP_ERR_TIMEOUT)
                            {
                                if (dec_frame_timeout > 0)
                                {
                                    dec_frame_timeout--;
                                    msleep(1);
                                    goto try_again;
                                }
                                mpp_err("%p decode_get_frame failed too much time\n", mpp_ctx);
                            }
                            if (ret)
                            {
                                mpp_err("%p decode_get_frame failed ret %d\n", ret, mpp_ctx);
                                break;
                            }

                            if (mpp_frame)
                            {
                                // 3.2.5 配置MppBufferGroup 并 通知解码器
                                // MPP_DEC_SET_INFO_CHANGE_READY
                                // 半内部分配模式
                                // 可通过mpp_buffer_group_limit_config限制解码器的内存使用量
                                if (mpp_frame_get_info_change(mpp_frame))
                                {
                                    RK_U32 width = mpp_frame_get_width(mpp_frame);
                                    RK_U32 height = mpp_frame_get_height(mpp_frame);
                                    RK_U32 hor_stride = mpp_frame_get_hor_stride(mpp_frame);
                                    RK_U32 ver_stride = mpp_frame_get_ver_stride(mpp_frame);
                                    RK_U32 buf_size = mpp_frame_get_buf_size(mpp_frame);

                                    mpp_log("%p decode_get_frame get info changed found\n", mpp_ctx);
                                    mpp_log(
                                        "%p decoder require buffer w:h [%d:%d] stride [%d:%d] "
                                        "buf_size %d",
                                        mpp_ctx, width, height, hor_stride, ver_stride, buf_size);

                                    mpp_grp = dec_buf_mgr_setup(mpp_buf_mgr, buf_size, 24, buf_mode);
                                    /* Set buffer to mpp decoder */
                                    ret = mpi->control(mpp_ctx, MPP_DEC_SET_EXT_BUF_GROUP, mpp_grp);
                                    if (ret)
                                    {
                                        mpp_err("%p set buffer group failed ret %d\n", mpp_ctx, ret);
                                        break;
                                    }
                                    ret = mpi->control(mpp_ctx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
                                    if (ret)
                                    {
                                        mpp_err("%p info change ready failed ret %d\n", mpp_ctx, ret);
                                        break;
                                    }
                                }
                                else
                                {
                                    // 3.2.6 解码所得帧
                                    char log_buf[256];
                                    RK_S32 log_size = sizeof(log_buf) - 1;
                                    RK_S32 log_len = 0;
                                    RK_U32 err_info = mpp_frame_get_errinfo(mpp_frame);
                                    RK_U32 discard = mpp_frame_get_discard(mpp_frame);

                                    if (!first_frm)
                                        first_frm = mpp_time();

                                    log_len += snprintf(log_buf + log_len, log_size - log_len,
                                                        "decode get frame %d", frame_count);

                                    if (err_info || discard)
                                    {
                                        log_len += snprintf(log_buf + log_len, log_size - log_len,
                                                            " err %x discard %x", err_info, discard);
                                    }
                                    // mpp_log("%p %s\n", mpp_ctx, log_buf);
                                    if (fp_output && !err_info)
                                    {
                                        /*spdlog::debug("frame timestamp: {}", get_frame_timestamp(frame_bgr));
                                        spdlog::debug("total elapsed time: {}", get_elapsed_time());
                                        spdlog::debug("=====================================");*/
                                        // 1. 构建 YUV Mat（注意 NV12 的排列：Y为前，UV为后）
                                        RK_U32 width = mpp_frame_get_hor_stride(mpp_frame);
                                        RK_U32 height = mpp_frame_get_ver_stride(mpp_frame);
                                        MppBuffer buffer = mpp_frame_get_buffer(mpp_frame);
                                        RK_U8 *base = (RK_U8 *)mpp_buffer_get_ptr(buffer);

                                        cv::Mat nv12(height + height / 2, width, CV_8UC1, base);
                                        // 2. 转换为 BGR 格式
                                        cv::Mat bgr;
                                        cv::cvtColor(nv12, bgr, cv::COLOR_YUV2BGR_NV12);
                                        // 3. 显示图像
                                        cv::imshow("NV12 Image", bgr);
                                        cv::waitKey(1);
                                        // cv::imwrite("test.jpg", bgr);

                                        // dump_mpp_frame_to_file(mpp_frame, fp_output);
                                    }

                                    frame_count++;
                                }
                                // frm_eos = mpp_frame_get_eos(mpp_frame);
                                mpp_frame_deinit(&mpp_frame);
                                get_frm = 1;
                            }

                            // try get runtime frame memory usage
                            if (mpp_grp)
                            {
                                size_t usage = mpp_buffer_group_usage(mpp_grp);
                                if (usage > max_usage)
                                    max_usage = usage;
                            }

                            // if last packet is send but last frame is not found continue
                            /*if (pkt_done && !frm_eos)
                            {
                                msleep(1);
                                continue;
                            }*/

                            /*if (frm_eos)
                            {
                                mpp_log("%p found last packet\n", mpp_ctx);
                                break;
                            }*/

                            // 解码指定帧数 或 解码完一帧
                            // 注意：当前是内部分帧模式，一个 packet 不一定正好包含一个 frame
                            /*if ((max_dec_frame_num > 0 && (frame_count >= max_dec_frame_num)) ||
                                ((max_dec_frame_num == 0) && frm_eos))
                                break;*/

                            // 内部分帧模式
                            if (get_frm)
                                continue;
                            break;

                        } while (1);

                        if (pkt_done)
                            break;

                        // 休眠等待，避免传入过快占满解码器内部队列
                        msleep(1);
                    } while (1);
                }

                // 延时(不然文件会立即全部播放完)
                // 定义时间基为微秒（AV_TIME_BASE）
                AVRational timeBase = {1, AV_TIME_BASE};
                // 将当前帧的时间戳（dts）从输入流的时间基转换为微秒时间基
                int64_t ptsTime = av_rescale_q(packet->dts, formatCtx->streams[videoStreamIndex]->time_base, timeBase);
                // 获取当前时间并减去播放开始时间，得到相对于播放开始时间的当前时间
                int64_t nowTime = av_gettime() - startTime;
                // 如果当前帧的显示时间还没到，就等待（睡眠）一段时间
                if (ptsTime > nowTime)
                {
                    spdlog::warn("当前帧的显示时间还没到, 等待（睡眠）一段时间");
                    av_usleep(ptsTime - nowTime);
                }

                av_packet_unref(packet);
            }
            else
            {
                error_frame_count += 1;
                char errbuf[AV_ERROR_MAX_STRING_SIZE];
                av_make_error_string(errbuf, AV_ERROR_MAX_STRING_SIZE, frameFinish);
                // std::cout << "av_read_frame error: " << frameFinish << ", " << errbuf << ", count:" << error_frame_count;
                spdlog::error("av_read_frame error: {}, errbuf: {}, count: {}", frameFinish, errbuf, error_frame_count);
                if (error_frame_count >= max_error_frame_count)
                {
                    spdlog::error("exit av_read_frame loop...");
                    throw std::runtime_error("av_read_frame error");
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(10)); // 如果读取帧数据失败，休眠一段时间再尝试
            }
        }
        catch (const std::exception &e)
        {
            std::cerr << "错误: " << e.what() << "，重新初始化...\n";
            rtsp_deinit();
            rtsp_initialized = -1; // 强制重新初始化
            error_frame_count = 0;
        }
    }
    rtsp_deinit();
}

bool MppRtspRecv::rtsp_init()
{
    return ffmpeg_init() && m_mpp_init();
}

void MppRtspRecv::rtsp_deinit()
{
    ffmpeg_deinit();
    m_mpp_deinit();
}

bool MppRtspRecv::ffmpeg_init()
{
    // 3.设置打开媒体文件的相关参数
    AVDictionary *options = nullptr;
    av_dict_set(&options, "buffer_size", "6M", 0); // 设置 buffer_size 为 2MB
    // 以tcp方式打开,如果以udp方式打开将tcp替换为udp
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    // 设置超时断开连接时间,单位微秒,3000000表示3秒
    av_dict_set(&options, "stimeout", "1000000", 0);
    // 设置最大时延,单位微秒,1000000表示1秒
    av_dict_set(&options, "max_delay", "1000000", 0);
    // 自动开启线程数
    av_dict_set(&options, "threads", "auto", 0);
    // 设置分析持续时间
    // av_dict_set(&options, "analyzeduration", "1000000", 0);
    // 设置探测大小
    av_dict_set(&options, "probesize", "5000000", 0);

    if (avformat_open_input(&formatCtx, url_.c_str(), nullptr, &options) != 0)
    {
        std::cerr << "Failed to open RTSP stream: " << url_ << std::endl;
        return false;
    }

    if (options != NULL)
    {
        av_dict_free(&options);
    }

    if (avformat_find_stream_info(formatCtx, nullptr) < 0)
    {
        std::cerr << "Failed to find stream info." << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    for (unsigned int i = 0; i < formatCtx->nb_streams; ++i)
    {
        if (formatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            videoStreamIndex = i;
            break;
        }
    }

    if (videoStreamIndex == -1)
    {
        std::cerr << "No video stream found." << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    AVCodec *codec = avcodec_find_decoder(formatCtx->streams[videoStreamIndex]->codecpar->codec_id);
    // AVCodec *codec = avcodec_find_decoder_by_name("hevc_rkmpp");
    if (!codec)
    {
        std::cerr << "Unsupported codec." << std::endl;
        avformat_close_input(&formatCtx);
        return false;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, formatCtx->streams[videoStreamIndex]->codecpar);

    if (avcodec_open2(codecCtx, codec, nullptr) < 0)
    {
        std::cerr << "Failed to open codec." << std::endl;
        avcodec_free_context(&codecCtx);
        avformat_close_input(&formatCtx);
        return false;
    }

    av_dump_format(formatCtx, videoStreamIndex, url_.c_str(), 0);

    packet = av_packet_alloc();

    return true;
}

void MppRtspRecv::ffmpeg_deinit()
{

    if (packet != nullptr)
    {
        av_packet_free(&packet);
        packet = nullptr;
    }

    if (formatCtx != nullptr)
    {
        avformat_close_input(&formatCtx);
        formatCtx = nullptr;
    }

    if (codecCtx)
    {
        avcodec_free_context(&codecCtx);
        codecCtx = nullptr;
    }
}

bool MppRtspRecv::m_mpp_init()
{
    // 1. 创建解码器
    int ret = mpp_create(&mpp_ctx, &mpi);
    if (ret)
    {
        mpp_err("mpp_create failed\n");
        return false;
    }

    // 2. 初始化解码器
    ret = mpp_init(mpp_ctx, MPP_CTX_DEC, video_type);
    if (ret)
    {
        mpp_err("%p mpp_init failed\n", mpp_ctx);
        return false;
    }

    mpp_dec_cfg_init(&mpp_cfg);
    // 2.1 获取默认配置
    ret = mpi->control(mpp_ctx, MPP_DEC_GET_CFG, mpp_cfg);

    // 2.2 设置内部分帧模式
    uint32_t need_split = 1;
    ret = mpp_dec_cfg_set_u32(mpp_cfg, "base:split_parse", need_split);
    if (ret)
    {
        mpp_err("%p failed to set split_parse ret %d\n", mpp_ctx, ret);
        return false;
    }
    ret = mpi->control(mpp_ctx, MPP_DEC_SET_CFG, mpp_cfg);
    if (ret)
    {
        mpp_err("%p failed to set cfg %p ret %d\n", mpp_ctx, mpp_cfg, ret);
        return false;
    }

    // 半内部模式配置
    ret = dec_buf_mgr_init(&mpp_buf_mgr);
    if (ret)
    {
        mpp_err("dec_buf_mgr_init failed\n");
        return false;
    }
    // 配置输入pkt
    ret = mpp_packet_init(&mpp_packet, NULL, 0);
    if (ret)
    {
        mpp_err("mpp_packet_init failed\n");
        return false;
    }

    if (save_dec)
    {
        fp_output = fopen(file_output, "w+b");
        if (NULL == fp_output)
        {
            mpp_err("failed to open output file %s\n", file_output);
            return false;
        }
    }

    return true;
}

void MppRtspRecv::m_mpp_deinit()
{
    // 5. 销毁解码器
    if (mpp_ctx)
    {
        mpp_destroy(mpp_ctx);
        mpp_ctx = NULL;
    }

    if (mpp_cfg)
    {
        mpp_dec_cfg_deinit(mpp_cfg);
        mpp_cfg = NULL;
    }

    if (mpp_packet)
    {
        mpp_packet_deinit(&mpp_packet);
        packet = NULL;
    }

    mpp_grp = NULL;
    if (mpp_buf_mgr)
    {
        dec_buf_mgr_deinit(mpp_buf_mgr);
        mpp_buf_mgr = NULL;
    }

    if (fp_output)
    {
        fclose(fp_output);
        fp_output = NULL;
    }
}
