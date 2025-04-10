#ifndef UTIL_H
#define UTIL_H

#pragma once

#include "rk_mpi.h"
#include <cstring>
#include <cstdio>

typedef void *FileReader;

#define MAX_FILE_NAME_LENGTH 256

typedef struct FileBufSlot_t
{
    RK_S32 index;
    MppBuffer buf;
    size_t size;
    RK_U32 eos;
    char *data;
} FileBufSlot;

typedef enum MppDecBufMode_e
{
    MPP_DEC_BUF_HALF_INT,
    MPP_DEC_BUF_INTERNAL,
    MPP_DEC_BUF_EXTERNAL,
    MPP_DEC_BUF_MODE_BUTT,
} MppDecBufMode;

typedef struct MpiDecTestCmd_t
{
    char file_input[MAX_FILE_NAME_LENGTH];
    char file_output[MAX_FILE_NAME_LENGTH];

    MppCodingType type;
    MppFrameFormat format;
    RK_U32 width;
    RK_U32 height;

    RK_U32 have_input;
    RK_U32 have_output;

    RK_U32 simple;
    RK_S32 timeout;
    RK_S32 frame_num;
    size_t pkt_size;
    MppDecBufMode buf_mode;

    /* use for mpi_dec_multi_test */
    RK_S32 nthreads;
    // report information
    size_t max_usage;

    /* data for share */
    FileReader reader;
    // FpsCalc fps;

    /* runtime log flag */
    RK_U32 quiet;
    RK_U32 trace_fps;
    char *file_slt;
} MpiDecTestCmd;

typedef struct
{
    MpiDecTestCmd *cmd;
    MppCtx ctx;
    MppApi *mpi;
    RK_U32 quiet;

    /* end of stream flag when set quit the loop */
    RK_U32 loop_end;

    /* input and output */
    // DecBufMgr buf_mgr;
    MppBufferGroup frm_grp;
    MppPacket packet;
    MppFrame frame;

    FILE *fp_output;
    RK_S32 frame_count;
    RK_S32 frame_num;

    RK_S64 first_pkt;
    RK_S64 first_frm;

    size_t max_usage;
    float frame_rate;
    RK_S64 elapsed_time;
    RK_S64 delay;
    FILE *fp_verify;
    // FrmCrc checkcrc;
} MpiDecLoopData;

void reader_init(FileReader *reader, char *file_in, MppCodingType type);
void reader_deinit(FileReader reader);

void reader_start(FileReader reader);
void reader_sync(FileReader reader);
void reader_stop(FileReader reader);

size_t reader_size(FileReader reader);
MPP_RET reader_read(FileReader reader, FileBufSlot **buf);
MPP_RET reader_index_read(FileReader reader, RK_S32 index, FileBufSlot **buf);
void reader_rewind(FileReader reader);

#endif