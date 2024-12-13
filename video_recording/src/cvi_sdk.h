#pragma once

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
}


#include "cvi_venc.h"
#include "sample_comm.h"

struct VideoInfo {
    int width = 1920;
    int height = 1080;
    int frame = 15;
    int rate = 128; // kbps
};

class EncoderCallback {
  public:
    virtual void onVideoData(AVPacket *pkt) = 0;
};

class CviVideoEncoder {
  public:
    CviVideoEncoder();
    ~CviVideoEncoder();

  public:
    int32_t init_cfg(const char *codeName, VideoInfo *pvideo);
    int32_t venc_start();

    int32_t getStream(EncoderCallback *pCallback);
    int32_t getStreamInternal(PAYLOAD_TYPE_E enType, VENC_STREAM_S *pstStream,
                              EncoderCallback *pCallback);
    int32_t sendkeyframe();

  protected:
    int32_t venc_initVi();
    int32_t initSysAndVb();
    int32_t venc_stop();
    void sys_vi_deinit();
    CVI_S32 _SAMPLE_COMM_VI_DefaultConfig();
    int32_t _SAMPLE_VENC_INIT_CHANNEL();
    CVI_S32 _SAMPLE_VENC_SetChnParam(uint32_t chgBitrate, int chgFramerate = 0);

  private:
    commonInputCfg *m_pCommonIc;
    vencChnCtx *m_pChnCtx;
    VENC_CHN m_chnNum = 0;
    SAMPLE_VI_CONFIG_S m_stViConfig;

    int m_duration;
};