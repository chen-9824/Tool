#ifndef MPPRTSPRECV_H
#define MPPRTSPRECV_H

#pragma once

#include "RTSPStream.h"

extern "C"
{

#include "rk_mpi.h"
#include "mpp_log.h"
#include "mpp_mem.h"
#include "mpp_debug.h"
#include "mpp_time.h"
#include "mpp_common.h"
}

typedef enum MppDecBufMode_e
{
    MPP_DEC_BUF_HALF_INT,
    MPP_DEC_BUF_INTERNAL,
    MPP_DEC_BUF_EXTERNAL,
    MPP_DEC_BUF_MODE_BUTT,
} MppDecBufMode;
typedef void *DecBufMgr;

class MppRtspRecv : public RTSPStream
{
public:
    MppRtspRecv(const std::string &url, int dst_width, int dst_height, AVPixelFormat dst_fmt);
    ~MppRtspRecv();

    bool startPlayer(player_type type) override;
    void stop() override;

private:
    void streamLoopWithMpp();
    bool rtsp_init();
    void rtsp_deinit();
    bool ffmpeg_init();
    void ffmpeg_deinit();
    bool m_mpp_init();
    void m_mpp_deinit();

private:
    MppCtx mpp_ctx = NULL;
    MppApi *mpi = NULL;
    MppCodingType video_type = MPP_VIDEO_CodingAVC;
    char video_path[100] = "/home/ido/chenFan/mpp/test/video/test.h264";
    // RK_S32 max_dec_frame_num = 100;  //最大解码帧数
    RK_S32 max_dec_frame_num = 0; // 解码完整个文件
    char file_output[100] = "/home/ido/chenFan/Tool/rkmpp/out.yuv";
    size_t max_usage; // 记录解码器最大使用内存

    FILE *fp_output = NULL; // 解码码流保存到文件
    bool save_dec = true;

    MppParam mpp_cfg = NULL;
    MppDecBufMode buf_mode = MPP_DEC_BUF_HALF_INT;
    MppBufferGroup mpp_grp = NULL;
    DecBufMgr mpp_buf_mgr;
    MppPacket mpp_packet = NULL;

    /* end of stream flag when set quit the loop */
    // RK_U32 loop_end = 0;
    RK_S32 frame_count = 0;
    RK_S64 first_pkt = 0;
    RK_S64 first_frm = 0;

    RK_S64 t_s, t_e;
};

#endif