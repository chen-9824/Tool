#include <iostream>

#include "rk_mpi.h"
#include "util.h"
int dec_decode()
{
    // base flow context
    MppCtx ctx = NULL;
    MppApi *mpi = NULL;

    // input / output
    MppPacket packet = NULL;
    MppFrame frame = NULL;

    // paramter for resource malloc
    RK_U32 width = cmd->width;
    RK_U32 height = cmd->height;
    MppCodingType type = cmd->type;

    // config for runtime mode
    MppDecCfg cfg = NULL;
    RK_U32 need_split = 1;

    // resources
    MppBuffer frm_buf = NULL;
    pthread_t thd;
    pthread_attr_t attr;
    MpiDecLoopData data;
    MPP_RET ret = MPP_OK;

    mpp_log("mpi_dec_test start\n");
    memset(&data, 0, sizeof(data));
    pthread_attr_init(&attr);

    cmd->simple = (cmd->type != MPP_VIDEO_CodingMJPEG) ? (1) : (0);

    if (cmd->have_output)
    {
        data.fp_output = fopen(cmd->file_output, "w+b");
        if (NULL == data.fp_output)
        {
            mpp_err("failed to open output file %s\n", cmd->file_output);
            goto MPP_TEST_OUT;
        }
    }

    if (cmd->file_slt)
    {
        data.fp_verify = fopen(cmd->file_slt, "wt");
        if (!data.fp_verify)
            mpp_err("failed to open verify file %s\n", cmd->file_slt);
    }

    ret = dec_buf_mgr_init(&data.buf_mgr);
    if (ret)
    {
        mpp_err("dec_buf_mgr_init failed\n");
        goto MPP_TEST_OUT;
    }

    if (cmd->simple)
    {
        ret = mpp_packet_init(&packet, NULL, 0);
        if (ret)
        {
            mpp_err("mpp_packet_init failed\n");
            goto MPP_TEST_OUT;
        }
    }
    else
    {
        RK_U32 hor_stride = MPP_ALIGN(width, 16);
        RK_U32 ver_stride = MPP_ALIGN(height, 16);

        ret = mpp_frame_init(&frame); /* output frame */
        if (ret)
        {
            mpp_err("mpp_frame_init failed\n");
            goto MPP_TEST_OUT;
        }

        data.frm_grp = dec_buf_mgr_setup(data.buf_mgr, hor_stride * ver_stride * 4, 4, cmd->buf_mode);
        if (!data.frm_grp)
        {
            mpp_err("failed to get buffer group for input frame ret %d\n", ret);
            ret = MPP_NOK;
            goto MPP_TEST_OUT;
        }

        /*
         * NOTE: For jpeg could have YUV420 and YUV422 the buffer should be
         * larger for output. And the buffer dimension should align to 16.
         * YUV420 buffer is 3/2 times of w*h.
         * YUV422 buffer is 2 times of w*h.
         * So create larger buffer with 2 times w*h.
         */
        ret = mpp_buffer_get(data.frm_grp, &frm_buf, hor_stride * ver_stride * 4);
        if (ret)
        {
            mpp_err("failed to get buffer for input frame ret %d\n", ret);
            goto MPP_TEST_OUT;
        }

        mpp_frame_set_buffer(frame, frm_buf);
    }

    // decoder demo
    ret = mpp_create(&ctx, &mpi);
    if (ret)
    {
        mpp_err("mpp_create failed\n");
        goto MPP_TEST_OUT;
    }

    mpp_log("%p mpi_dec_test decoder test start w %d h %d type %d\n",
            ctx, width, height, type);

    ret = mpp_init(ctx, MPP_CTX_DEC, type);
    if (ret)
    {
        mpp_err("%p mpp_init failed\n", ctx);
        goto MPP_TEST_OUT;
    }

    mpp_dec_cfg_init(&cfg);

    /* get default config from decoder context */
    ret = mpi->control(ctx, MPP_DEC_GET_CFG, cfg);
    if (ret)
    {
        mpp_err("%p failed to get decoder cfg ret %d\n", ctx, ret);
        goto MPP_TEST_OUT;
    }

    /*
     * split_parse is to enable mpp internal frame spliter when the input
     * packet is not aplited into frames.
     */
    ret = mpp_dec_cfg_set_u32(cfg, "base:split_parse", need_split);
    if (ret)
    {
        mpp_err("%p failed to set split_parse ret %d\n", ctx, ret);
        goto MPP_TEST_OUT;
    }

    ret = mpi->control(ctx, MPP_DEC_SET_CFG, cfg);
    if (ret)
    {
        mpp_err("%p failed to set cfg %p ret %d\n", ctx, cfg, ret);
        goto MPP_TEST_OUT;
    }

    data.cmd = cmd;
    data.ctx = ctx;
    data.mpi = mpi;
    data.loop_end = 0;
    data.packet = packet;
    data.frame = frame;
    data.frame_count = 0;
    data.frame_num = cmd->frame_num;
    data.quiet = cmd->quiet;

    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

    ret = pthread_create(&thd, &attr, thread_decode, &data);
    if (ret)
    {
        mpp_err("failed to create thread for input ret %d\n", ret);
        goto MPP_TEST_OUT;
    }

    if (cmd->frame_num < 0)
    {
        // wait for input then quit decoding
        mpp_log("*******************************************\n");
        mpp_log("**** Press Enter to stop loop decoding ****\n");
        mpp_log("*******************************************\n");

        getc(stdin);
        data.loop_end = 1;
    }

    pthread_join(thd, NULL);

    cmd->max_usage = data.max_usage;

    ret = mpi->reset(ctx);
    if (ret)
    {
        mpp_err("%p mpi->reset failed\n", ctx);
        goto MPP_TEST_OUT;
    }

MPP_TEST_OUT:
    if (data.packet)
    {
        mpp_packet_deinit(&data.packet);
        data.packet = NULL;
    }

    if (frame)
    {
        mpp_frame_deinit(&frame);
        frame = NULL;
    }

    if (ctx)
    {
        mpp_destroy(ctx);
        ctx = NULL;
    }

    if (!cmd->simple)
    {
        if (frm_buf)
        {
            mpp_buffer_put(frm_buf);
            frm_buf = NULL;
        }
    }

    data.frm_grp = NULL;
    if (data.buf_mgr)
    {
        dec_buf_mgr_deinit(data.buf_mgr);
        data.buf_mgr = NULL;
    }

    if (data.fp_output)
    {
        fclose(data.fp_output);
        data.fp_output = NULL;
    }

    if (data.fp_verify)
    {
        fclose(data.fp_verify);
        data.fp_verify = NULL;
    }

    if (cfg)
    {
        mpp_dec_cfg_deinit(cfg);
        cfg = NULL;
    }

    pthread_attr_destroy(&attr);

    return ret;
}

int main(int, char **)
{
    std::cout << "Hello, from RKMPP!\n";
}
