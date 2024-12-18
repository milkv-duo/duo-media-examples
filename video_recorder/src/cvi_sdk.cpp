#include "cvi_sdk.h"

#include "log.h"
#include <cstring>
#include <stdio.h>

int32_t find_pre_start_code(uint8_t *buf, int32_t bufLen) {
    int32_t i = 0;
    while (i < bufLen - 3) {
        if (buf[i] == 0 && buf[i + 1] == 0 && buf[i + 2] == 0 &&
            buf[i + 3] == 1) {
            return i;
            break;
        }
        i++;
    }
    return -1;
}

PIXEL_FORMAT_E vencMapPixelFormat(CVI_S32 pixel_format) {
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;

    switch (pixel_format) {
    case 0:
        enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
        break;
    case 1:
        enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_422;
        break;
    case 2:
        enPixelFormat = PIXEL_FORMAT_NV12;
        break;
    case 3:
        enPixelFormat = PIXEL_FORMAT_NV21;
        break;
    default:
        WARN("Unknown input pixel format. Assume YUV420P.\n");
        enPixelFormat = PIXEL_FORMAT_YUV_PLANAR_420;
        break;
    }

    return enPixelFormat;
}

PIC_SIZE_E getEnSize(CVI_U32 u32Width, CVI_U32 u32Height) {
    if (u32Width == 352 && u32Height == 288)
        return PIC_CIF;
    else if (u32Width == 720 && u32Height == 576)
        return PIC_D1_PAL;
    else if (u32Width == 720 && u32Height == 480)
        return PIC_D1_NTSC;
    else if (u32Width == 1280 && u32Height == 720)
        return PIC_720P;
    else if (u32Width == 1920 && u32Height == 1080)
        return PIC_1080P;
    else if (u32Width == 2592 && u32Height == 1520)
        return PIC_2592x1520;
    else if (u32Width == 2592 && u32Height == 1536)
        return PIC_2592x1536;
    else if (u32Width == 2592 && u32Height == 1944)
        return PIC_2592x1944;
    else if (u32Width == 2716 && u32Height == 1524)
        return PIC_2716x1524;
    else if (u32Width == 3840 && u32Height == 2160)
        return PIC_3840x2160;
    else if (u32Width == 4096 && u32Height == 2160)
        return PIC_4096x2160;
    else if (u32Width == 3000 && u32Height == 3000)
        return PIC_3000x3000;
    else if (u32Width == 4000 && u32Height == 3000)
        return PIC_4000x3000;
    else if (u32Width == 3840 && u32Height == 8640)
        return PIC_3840x8640;
    else if (u32Width == 640 && u32Height == 480)
        return PIC_640x480;
    else
        return PIC_CUSTOMIZE;
}

// 转换函数
std::string SampleRcToString(int rc) {
    switch (rc) {
    case SAMPLE_RC_CBR:
        return "CBR";
    case SAMPLE_RC_VBR:
        return "VBR";
    case SAMPLE_RC_AVBR:
        return "AVBR";
    case SAMPLE_RC_QVBR:
        return "QVBR";
    case SAMPLE_RC_FIXQP:
        return "FIXQP";
    case SAMPLE_RC_QPMAP:
        return "QPMAP";
    case SAMPLE_RC_UBR:
        return "UBR";
    default:
        return "Unknown";
    }
}

CVI_U32 getSrcFrameSizeByPixelFormat(CVI_U32 width, CVI_U32 height,
                                     PIXEL_FORMAT_E enPixelFormat) {
    CVI_U32 size = 0;

    switch (enPixelFormat) {
    case PIXEL_FORMAT_YUV_PLANAR_422:
        size = width * height * 2;
        break;
    case PIXEL_FORMAT_YUV_PLANAR_420:
    case PIXEL_FORMAT_NV12:
    case PIXEL_FORMAT_NV21:
        size = width * height * 3 / 2;
        break;
    default:
        WARN("Unknown pixel format. Assume YUV420P.");
        size = width * height * 3 / 2;
        break;
    }

    return size;
}

CVI_S32 free_frame(VIDEO_FRAME_INFO_S *pstVideoFrame) {
    VIDEO_FRAME_S *pstVFrame = &pstVideoFrame->stVFrame;
    VB_BLK blk;

    if (pstVFrame->pu8VirAddr[0])
        CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[0],
                       pstVFrame->u32Length[0]);
    if (pstVFrame->pu8VirAddr[1])
        CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[1],
                       pstVFrame->u32Length[1]);
    if (pstVFrame->pu8VirAddr[2])
        CVI_SYS_Munmap((CVI_VOID *)pstVFrame->pu8VirAddr[2],
                       pstVFrame->u32Length[2]);

    blk = CVI_VB_PhysAddr2Handle(pstVFrame->u64PhyAddr[0]);
    if (blk != VB_INVALID_HANDLE) {
        CVI_VB_ReleaseBlock(blk);
    }

    free(pstVideoFrame);

    return CVI_SUCCESS;
}
void initInputCfg(commonInputCfg *pcic, chnInputCfg *pIc) {
    SAMPLE_COMM_VENC_InitCommonInputCfg(pcic);
    SAMPLE_COMM_VENC_InitChnInputCfg(pIc);
}

CVI_S32 SAMPLE_COMM_VENC_GetDataType(PAYLOAD_TYPE_E enType,
                                     VENC_PACK_S *ppack) {
    if (enType == PT_H264)
        return ppack->DataType.enH264EType;
    else if (enType == PT_H265)
        return ppack->DataType.enH265EType;
    else if (enType == PT_JPEG || enType == PT_MJPEG)
        return ppack->DataType.enJPEGEType;

    return CVI_FAILURE;
}

CVI_S32 checkInputCfg(chnInputCfg *pIc) {
    INFO("----------checkInputCfg----------->>>");
    if (strcmp(pIc->codec, "264") && strcmp(pIc->codec, "265")) {
        ERROR("codec = {0}", pIc->codec);
        return CVI_FAILURE;
    }

    INFO("framerate = {0}", pIc->framerate);

    if (pIc->gop < 1) {
        if (!strcmp(pIc->codec, "264"))
            pIc->gop = DEF_264_GOP;
        else
            pIc->gop = DEF_GOP;
    }
    INFO("gop = {0}", pIc->gop);

    if (!strcmp(pIc->codec, "265")) {
        if (pIc->single_LumaBuf > 0) {
            ERROR("single_LumaBuf only supports H.264");
            pIc->single_LumaBuf = 0;
        }
    }
    pIc->iqp = (pIc->iqp >= 0) ? pIc->iqp : DEF_IQP;
    pIc->pqp = (pIc->pqp >= 0) ? pIc->pqp : DEF_PQP;

    std::string odl_str_rcMode = SampleRcToString(pIc->rcMode);
    if (pIc->rcMode == -1) {
        pIc->rcMode = SAMPLE_RC_FIXQP;
    }

    std::string str_rcMode = SampleRcToString(pIc->rcMode);

    INFO("rcMode: {0} --> {1}", odl_str_rcMode, str_rcMode);
    if (pIc->rcMode == SAMPLE_RC_CBR || pIc->rcMode == SAMPLE_RC_VBR ||
        pIc->rcMode == SAMPLE_RC_AVBR || pIc->rcMode == SAMPLE_RC_QPMAP ||
        pIc->rcMode == SAMPLE_RC_UBR) {

        if (pIc->rcMode == SAMPLE_RC_CBR || pIc->rcMode == SAMPLE_RC_UBR) {
            if (pIc->bitrate <= 0) {
                ERROR("CBR / UBR bitrate must be not less than 0");
                return -1;
            }
            INFO("bitrate = {0}", pIc->bitrate);
        } else if (pIc->rcMode == SAMPLE_RC_VBR) {
            if (pIc->maxbitrate <= 0) {
                ERROR("VBR must be not less than 0");
                return -1;
            }
            INFO("RC_VBR, maxbitrate = {0}", pIc->maxbitrate);
        }

        pIc->firstFrmstartQp =
            (pIc->firstFrmstartQp < 0 || pIc->firstFrmstartQp > 51)
                ? 63
                : pIc->firstFrmstartQp;
        INFO("firstFrmstartQp = {0}", pIc->firstFrmstartQp);

        pIc->maxIqp = (pIc->maxIqp >= 0) ? pIc->maxIqp : DEF_264_MAXIQP;
        pIc->minIqp = (pIc->minIqp >= 0) ? pIc->minIqp : DEF_264_MINIQP;
        pIc->maxQp = (pIc->maxQp >= 0) ? pIc->maxQp : DEF_264_MAXQP;
        pIc->minQp = (pIc->minQp >= 0) ? pIc->minQp : DEF_264_MINQP;
        INFO("maxQp = {0}, minQp = {1}, maxIqp = {2}, minIqp = {3}", pIc->maxQp,
             pIc->minQp, pIc->maxIqp, pIc->minIqp);

        if (pIc->statTime == 0) {
            pIc->statTime = DEF_STAT_TIME;
        }
        INFO("statTime = {0}", pIc->statTime);
    } else if (pIc->rcMode == SAMPLE_RC_FIXQP) {
        if (pIc->firstFrmstartQp != -1) {
            WARN("firstFrmstartQp is invalid in FixQP mode");
            pIc->firstFrmstartQp = -1;
        }

        pIc->bitrate = 0;
        INFO("RC_FIXQP, iqp = {0}, pqp = {1}", pIc->iqp, pIc->pqp);
    } else {
        ERROR("codec = {0}, rcMode = {1}, not supported RC mode", pIc->codec,
              pIc->rcMode);
        return -1;
    }
    INFO("bitrate: {0} kbps", pIc->bitrate);
    return 0;
}

CVI_S32 CviVideoEncoder::_SAMPLE_COMM_VI_DefaultConfig(CVI_VOID) {
    SAMPLE_INI_CFG_S stIniCfg;
    SAMPLE_VI_CONFIG_S stViConfig;

    memset(&stIniCfg, 0, sizeof(stIniCfg));
    memset(&stViConfig, 0, sizeof(stViConfig));

    PIC_SIZE_E enPicSize;
    SIZE_S stSize;
    CVI_S32 s32Ret = CVI_SUCCESS;

    stIniCfg.enSource = VI_PIPE_FRAME_SOURCE_DEV;
    stIniCfg.devNum = 1;
    stIniCfg.enSnsType[0] = SONY_IMX327_MIPI_2M_30FPS_12BIT;
    stIniCfg.enWDRMode[0] = WDR_MODE_NONE;
    stIniCfg.s32BusId[0] = 3;
    stIniCfg.s32SnsI2cAddr[0] = -1;
    stIniCfg.MipiDev[0] = 0xFF;
    stIniCfg.u8UseMultiSns = 0;

    // Get config from ini if found.
    if (SAMPLE_COMM_VI_ParseIni(&stIniCfg)) {
        SAMPLE_PRT("Parse complete\n");
    }

    /************************************************
     * step1:  Config VI
     ************************************************/
    s32Ret = SAMPLE_COMM_VI_IniToViCfg(&stIniCfg, &stViConfig);
    if (s32Ret != CVI_SUCCESS)
        return s32Ret;

    /************************************************
     * step2:  Get input size
     ************************************************/
    s32Ret = SAMPLE_COMM_VI_GetSizeBySensor(stIniCfg.enSnsType[0], &enPicSize);
    if (s32Ret != CVI_SUCCESS) {
        CVI_TRACE_LOG(CVI_DBG_ERR,
                      "SAMPLE_COMM_VI_GetSizeBySensor failed with %#x\n",
                      s32Ret);
        return s32Ret;
    }

    s32Ret = SAMPLE_COMM_SYS_GetPicSize(enPicSize, &stSize);
    if (s32Ret != CVI_SUCCESS) {
        CVI_TRACE_LOG(CVI_DBG_ERR,
                      "SAMPLE_COMM_SYS_GetPicSize failed with %#x\n", s32Ret);
        return s32Ret;
    }

    /************************************************
     * step3:  Init modules
     ************************************************/
    s32Ret = SAMPLE_PLAT_SYS_INIT(stSize);
    if (s32Ret != CVI_SUCCESS) {
        CVI_TRACE_LOG(CVI_DBG_ERR, "sys init failed. s32Ret: 0x%x !\n", s32Ret);
        return s32Ret;
    }

    if (stIniCfg.enSource == VI_PIPE_FRAME_SOURCE_DEV) {
        s32Ret = SAMPLE_PLAT_VI_INIT(&stViConfig);
        if (s32Ret != CVI_SUCCESS) {
            CVI_TRACE_LOG(CVI_DBG_ERR, "vi init failed. s32Ret: 0x%x !\n",
                          s32Ret);
            return s32Ret;
        }
    }

    memcpy(&m_stViConfig, &stViConfig, sizeof(SAMPLE_VI_CONFIG_S));
    // memcpy(&g_stIniCfg, &stIniCfg, sizeof(SAMPLE_INI_CFG_S));

    return s32Ret;
}

void CviVideoEncoder::sys_vi_deinit() {
    SAMPLE_COMM_VI_DestroyIsp(&m_stViConfig);

    SAMPLE_COMM_VI_DestroyVi(&m_stViConfig);

    SAMPLE_COMM_SYS_Exit();
}

CviVideoEncoder::CviVideoEncoder() {
    m_pCommonIc = new commonInputCfg;
    m_pChnCtx = new vencChnCtx;
}

CviVideoEncoder::~CviVideoEncoder() {
    venc_stop();
    sys_vi_deinit();

    delete m_pCommonIc;
    delete m_pChnCtx;
}

int32_t CviVideoEncoder::init_cfg(const char *codeName, VideoInfo *pvideo) {
    // 设置 log 等级
    cviGetMask();

    chnInputCfg *pIc = &(m_pChnCtx->chnIc);
    initInputCfg(m_pCommonIc, pIc);

    strncpy(pIc->codec, codeName, sizeof(codeName) - 1);
    pIc->codec[sizeof(codeName) - 1] = '\0';
    pIc->width = pvideo->width;
    pIc->height = pvideo->height;
    pIc->bsMode = BS_MODE_QUERY_STAT;
    pIc->framerate = pvideo->frame;
    pIc->bitrate = pvideo->rate;
    pIc->pixel_format = 3; // Nv21
    // pIc->rcMode = SAMPLE_RC_AVBR;

    m_pCommonIc->bindmode = VENC_BIND_VI;
    m_pCommonIc->numChn = 1;
    m_pCommonIc->bThreadDisable = CVI_TRUE;
    m_pCommonIc->u32ViWidth = pvideo->width;
    m_pCommonIc->u32ViHeight = pvideo->height;


    int ret = checkInputCfg(pIc);
    if (ret < 0) {
        ERROR(" checkInputCfg failed! ret: {0}", ret);
        return -1;
    }

    //m_duration = static_cast<int>(AV_TIME_BASE / pIc->framerate);

    venc_initVi();

    return 0;
}

int32_t CviVideoEncoder::venc_initVi() {
    chnInputCfg *pIc = &(m_pChnCtx->chnIc);
    commonInputCfg *pcic = m_pCommonIc;
    CVI_S32 s32Ret = CVI_SUCCESS;

    if (pcic->u32ViWidth == 0 || pcic->u32ViHeight == 0) {
        return s32Ret;
    }

    pcic->ifInitVb = 0;

    // setup VI
    s32Ret = _SAMPLE_COMM_VI_DefaultConfig();
    if (s32Ret != CVI_SUCCESS) {
        ERROR("SAMPLE_COMM_VI_DefaultConfig failure! ret = {0}", s32Ret);
        return s32Ret;
    }
    sleep(2);

    pIc->bind_mode = pcic->bindmode;

    return s32Ret;
}

int32_t CviVideoEncoder::venc_start() {

    chnInputCfg *pIc = &(m_pChnCtx->chnIc);

    CVI_S32 s32Ret = CVI_SUCCESS;

    if (strcmp(pIc->codec, "264") && strcmp(pIc->codec, "265")) {
        ERROR("codec = {0}", pIc->codec);
        return CVI_FAILURE;
    }

    if (m_pCommonIc->ifInitVb) {
        s32Ret = initSysAndVb();
        if (s32Ret != CVI_SUCCESS) {
            ERROR("fail to init sys and vb, {0}", s32Ret);
            return s32Ret;
        }
    }

    s32Ret = SAMPLE_COMM_VENC_SetModParam(m_pCommonIc);
    if (s32Ret != CVI_SUCCESS) {
        ERROR("SAMPLE_COMM_VENC_SetModParam failure!");
        return CVI_FAILURE;
    }

    s32Ret = _SAMPLE_VENC_INIT_CHANNEL();
    if (s32Ret) {
        ERROR("Chn 0 init_channel  failed! ret = {0}", s32Ret);
        return CVI_FAILURE;
    }

    return CVI_SUCCESS;
}

int32_t CviVideoEncoder::initSysAndVb() {
    chnInputCfg *pIc = &(m_pChnCtx->chnIc);

    CVI_S32 s32Ret = CVI_SUCCESS;
    CVI_U32 u32BlkSize;
    VB_CONFIG_S stVbConf;

    memset(&stVbConf, 0, sizeof(VB_CONFIG_S));

    stVbConf.u32MaxPoolCnt = 0;

    SIZE_S stSize;
    PIXEL_FORMAT_E enPixelFormat;
    CVI_BOOL bRepeated = CVI_FALSE;

    enPixelFormat = vencMapPixelFormat(pIc->pixel_format);

    if (pIc->inWidth || pIc->inHeight) {
        stSize.u32Width = pIc->inWidth;
        stSize.u32Height = pIc->inHeight;
    } else {
        stSize.u32Width = pIc->width;
        stSize.u32Height = pIc->height;
    }
    u32BlkSize =
        VENC_GetPicBufferSize(stSize.u32Width, stSize.u32Height, enPixelFormat,
                              DATA_BITWIDTH_8, COMPRESS_MODE_NONE);
#ifdef ARCH_CV183X
    INFO("arch183x u32BlkSize[{0}] align[{1}]", u32BlkSize, VENC_ALIGN_W);
#else
    u32BlkSize += 0x1000 * 3;
    INFO("arch182x u32BlkSize[{0}] align[{1}]", u32BlkSize, VENC_ALIGN_W);
#endif

    for (CVI_U32 j = 0; j < stVbConf.u32MaxPoolCnt; j++) {
        if (u32BlkSize == stVbConf.astCommPool[j].u32BlkSize) {
            stVbConf.astCommPool[j].u32BlkCnt++;
            bRepeated = CVI_TRUE;
            break;
        }
    }

    if (bRepeated == CVI_FALSE) {
        stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkSize = u32BlkSize;
        stVbConf.astCommPool[stVbConf.u32MaxPoolCnt].u32BlkCnt = 1;
        stVbConf.u32MaxPoolCnt++;
    }

    if (stVbConf.u32MaxPoolCnt > VB_MAX_COMM_POOLS) {
        ERROR("u32MaxPoolCnt {0} > VB_MAX_COMM_POOLS {1}",
              stVbConf.u32MaxPoolCnt, VB_MAX_COMM_POOLS);
        return CVI_FAILURE;
    }

    for (CVI_U32 j = 0; j < stVbConf.u32MaxPoolCnt; j++) {
        INFO("[Pool {0}] u32BlkSize: {1}, u32BlkCnt: {2}", j,
             stVbConf.astCommPool[j].u32BlkSize,
             stVbConf.astCommPool[j].u32BlkCnt);
    }

    s32Ret = SAMPLE_COMM_SYS_Init(&stVbConf);
    if (s32Ret != CVI_SUCCESS) {
        ERROR("SAMPLE_COMM_SYS_Init failed! ret: {0}", s32Ret);
        return s32Ret;
    }

    return s32Ret;
}

int32_t CviVideoEncoder::_SAMPLE_VENC_INIT_CHANNEL() {
    chnInputCfg *pIc = &(m_pChnCtx->chnIc);
    CVI_S32 s32Ret = CVI_SUCCESS;
    SIZE_S inFrmSize;

    m_pChnCtx->enPixelFormat = vencMapPixelFormat(pIc->pixel_format);
    m_pChnCtx->VencChn = m_chnNum;
    m_pChnCtx->enGopMode = (VENC_GOP_MODE_E)pIc->gopMode;
    m_pChnCtx->s32FbCnt = 1;

    // 10000
    m_pChnCtx->chnIc.getstream_timeout = -1;

    if (!strcmp(pIc->codec, "265"))
        m_pChnCtx->enPayLoad = PT_H265;
    else if (!strcmp(pIc->codec, "264"))
        m_pChnCtx->enPayLoad = PT_H264;

    if (m_pChnCtx->enPayLoad == PT_H264)
        m_pChnCtx->u32Profile = pIc->u32Profile;
    else
        m_pChnCtx->u32Profile = 0;

    m_pChnCtx->stSize.u32Width = pIc->width;
    m_pChnCtx->stSize.u32Height = pIc->height;

    if (pIc->inWidth || pIc->inHeight) {
        inFrmSize.u32Width = pIc->inWidth;
        inFrmSize.u32Height = pIc->inHeight;
    } else {
        inFrmSize = m_pChnCtx->stSize;
    }

    m_pChnCtx->u32FrameSize = getSrcFrameSizeByPixelFormat(
        m_pChnCtx->stSize.u32Width, m_pChnCtx->stSize.u32Height,
        m_pChnCtx->enPixelFormat);

    m_pChnCtx->enRcMode = (SAMPLE_RC_E)pIc->rcMode;
    /*
    INFO("enPayLoad = {0}, enRcMode = {1}, bsMode = {2}", m_pChnCtx->enPayLoad,
         m_pChnCtx->enRcMode, pIc->bsMode);
         */

    m_pChnCtx->enSize = getEnSize(pIc->width, pIc->height);
    if (m_pChnCtx->enSize == PIC_BUTT) {
        ERROR("unsupport size {0} x {1}\n", pIc->width, pIc->height);
        return CVI_FAILURE;
    }

    if (m_pChnCtx->enPayLoad != PT_JPEG) {
        s32Ret = SAMPLE_COMM_VENC_GetGopAttr(m_pChnCtx->enGopMode,
                                             &m_pChnCtx->stGopAttr);
        if (s32Ret != CVI_SUCCESS) {
            ERROR("Venc Get GopAttr for {0}!", s32Ret);
            return CVI_FAILURE;
        }

        if ((m_pChnCtx->enPayLoad != PT_JPEG) &&
            (m_pChnCtx->enPayLoad != PT_MJPEG)) {
            if (m_pCommonIc->vbMode == VB_SOURCE_USER) {
                // create vb pool
                TRACE("vbMode VB_SOURCE_USER_MODE");
                s32Ret = SAMPLE_COMM_VENC_InitVBPool(m_pChnCtx, m_chnNum);
                if (s32Ret != CVI_SUCCESS)
                    ERROR("init mod common vb fail");
                // should attach vb pool after chn create
            }
        }
    }

    s32Ret = SAMPLE_COMM_VENC_Start(pIc, m_pChnCtx->VencChn,
                                    m_pChnCtx->enPayLoad, m_pChnCtx->enSize,
                                    m_pChnCtx->enRcMode, m_pChnCtx->u32Profile,
                                    CVI_FALSE, &m_pChnCtx->stGopAttr);
    if (s32Ret != CVI_SUCCESS) {
        ERROR("Venc Start failed for {0}!\n", s32Ret);
        return CVI_FAILURE;
    }

    /*
    if (m_pCommonIc->ifInitVb) {
        m_pChnCtx->pstFrameInfo =
            allocate_frame(inFrmSize, pvecc->enPixelFormat);
        INFO("pstFrameInfo = {0}\n", m_pChnCtx->pstFrameInfo);

        if (!m_pChnCtx->pstFrameInfo) {
            CVI_VENC_ERR("allocate_frame\n");
            return CVI_FAILURE;
        }

        m_pChnCtx->pstVFrame = &m_pChnCtx->pstFrameInfo->stVFrame;
    }
    */

    return CVI_SUCCESS;
}

int32_t CviVideoEncoder::venc_stop() {
    commonInputCfg *pcic = m_pCommonIc;
    chnInputCfg *pIc = &(m_pChnCtx->chnIc);
    vencChnCtx *pvecc = m_pChnCtx;

    if (strcmp(pIc->codec, "264") && strcmp(pIc->codec, "265")) {
        ERROR("codec = {0}", pIc->codec);
        return CVI_FAILURE;
    }

    SAMPLE_COMM_VI_UnBind_VENC(0, m_chnNum, m_chnNum);

    SAMPLE_COMM_VENC_Stop(m_chnNum);

    if (pcic->ifInitVb && pvecc->pstFrameInfo != nullptr) {
        free_frame(pvecc->pstFrameInfo);
    }

    if (pvecc->pu8QpMap) {
        free(pvecc->pu8QpMap);
        pvecc->pu8QpMap = NULL;
    }

    return 0;
}

int32_t CviVideoEncoder::getStream(EncoderCallback *pCallback) {
    VENC_CHN_STATUS_S stStat;
    VENC_CHN_ATTR_S stVencChnAttr;
    VENC_STREAM_S stStream;

    vencChnCtx *pvecc = m_pChnCtx;
    VENC_CHN VencChn = pvecc->VencChn;
    chnInputCfg *pIc = &pvecc->chnIc;
    CVI_S32 s32Ret;

    do {
        s32Ret = CVI_VENC_GetChnAttr(VencChn, &stVencChnAttr);
        if (s32Ret != CVI_SUCCESS) {
            ERROR("CVI_VENC_GetChnAttr, VencChn = {0}, s32Ret = {1}", VencChn,
                  s32Ret);
            return s32Ret;
        }

        s32Ret = CVI_VENC_QueryStatus(VencChn, &stStat);
        if (s32Ret != CVI_SUCCESS) {
            ERROR("CVI_VENC_QueryStatus, Vench = {0}, s32Ret = {1}", VencChn,
                  s32Ret);
            return s32Ret;
        }

        if (!stStat.u32CurPacks) {
            ERROR("u32CurPacks = NULL!");
            return s32Ret;
        }

        stStream.pstPack =
            (VENC_PACK_S *)malloc(sizeof(VENC_PACK_S) * stStat.u32CurPacks);
        if (stStream.pstPack == NULL) {
            ERROR("malloc memory failed!");
            return s32Ret;
        }
    RETRY_GET_STREAM:
        s32Ret = CVI_VENC_GetStream(VencChn, &stStream, pIc->getstream_timeout);
        if (s32Ret != CVI_SUCCESS) {
            if (s32Ret == CVI_ERR_VENC_BUSY) {
                WARN("CVI_VENC_GetStream, VencChn Retry= {0},s32Ret = {1}",
                     VencChn, s32Ret);
                goto RETRY_GET_STREAM;
            } else {
                WARN("CVI_VENC_GetStream, VencChn = {0}, s32Ret = {1}", VencChn,
                     s32Ret);
                break;
            }
        }

        s32Ret = getStreamInternal(stVencChnAttr.stVencAttr.enType, &stStream,
                                   pCallback);

        if (s32Ret != CVI_SUCCESS) {
            ERROR("SAMPLE_COMM_VENC_SaveStream, s32Ret = {0}", s32Ret);
            break;
        }

        s32Ret = CVI_VENC_ReleaseStream(VencChn, &stStream);
        if (s32Ret != CVI_SUCCESS) {
            ERROR("CVI_VENC_ReleaseStream, s32Ret = {0}", s32Ret);
            break;
        }
    } while (CVI_FALSE);

    free(stStream.pstPack);
    stStream.pstPack = NULL;

    return s32Ret;
}

CVI_S32 CviVideoEncoder::getStreamInternal(PAYLOAD_TYPE_E enType,
                                           VENC_STREAM_S *pstStream,
                                           EncoderCallback *pCallback) {
    VENC_PACK_S *ppack;

    int32_t src_nb = 0;
    uint8_t *src_payload = NULL;

    int32_t nb = 0;
    uint8_t *payload = NULL;
    int32_t startCodePos = 0;

    for (CVI_U32 i = 0; i < pstStream->u32PackCount; i++) {
        AVPacket *pkt = av_packet_alloc();

        ppack = &pstStream->pstPack[i];
        src_nb = ppack->u32Len - ppack->u32Offset;
        src_payload = ppack->pu8Addr + ppack->u32Offset;

        startCodePos = find_pre_start_code(src_payload, src_nb);
        if (startCodePos == -1) {
            continue;
        }

        payload = src_payload + startCodePos;
        nb = src_nb - (startCodePos);

        if (ppack->DataType.enH264EType == H264E_NALU_SPS) {
            // TODO
        }

        if (ppack->DataType.enH264EType == H264E_NALU_PPS) {
            // TODO
        }

        if (ppack->DataType.enH264EType == H264E_NALU_IDRSLICE ||
            ppack->DataType.enH264EType == H264E_NALU_ISLICE) {
            // TODO
            pkt->flags |= AV_PKT_FLAG_KEY;
        }

        if (ppack->DataType.enH264EType == H264E_NALU_PSLICE) {
            // TODO
        }

        pkt->data = payload;
        pkt->size = nb;
        pkt->pts = ppack->u64PTS; // 设置时间戳（按需调整）
        pkt->dts = pkt->pts;             // 设置解码时间戳（按需调整）

        pCallback->onVideoData(pkt);
        av_packet_free(&pkt);
    }

    return CVI_SUCCESS;
}

int32_t CviVideoEncoder::sendkeyframe() {
    CVI_S32 s32Ret = CVI_VENC_RequestIDR(m_pChnCtx->VencChn, true);
    return s32Ret;
}

CVI_S32 CviVideoEncoder::_SAMPLE_VENC_SetChnParam(uint32_t chgBitrate,
                                                  int chgFramerate) {
    CVI_S32 s32Ret = CVI_SUCCESS;
    VENC_CHN VencChn = m_pChnCtx->VencChn;
    chnInputCfg *pIc = &m_pChnCtx->chnIc;

    VENC_CHN_ATTR_S stChnAttr;

    CVI_VENC_GetChnAttr(VencChn, &stChnAttr);

    if (chgBitrate <= 0) {
        chgBitrate = pIc->chgBitrate;
    }

    if (chgFramerate <= 0) {
        chgFramerate = pIc->chgFramerate;
    }

    if (stChnAttr.stVencAttr.enType == PT_H265) {
        stChnAttr.stRcAttr.stH265Cbr.u32BitRate = pIc->chgBitrate = chgBitrate;
        stChnAttr.stRcAttr.stH265Cbr.fr32DstFrameRate = pIc->chgFramerate =
            chgFramerate;
    } else if (stChnAttr.stVencAttr.enType == PT_H264) {
        stChnAttr.stRcAttr.stH264Cbr.u32BitRate = pIc->chgBitrate = chgBitrate;
        stChnAttr.stRcAttr.stH264Cbr.fr32DstFrameRate = pIc->chgFramerate =
            chgFramerate;
    } else if (stChnAttr.stVencAttr.enType == PT_MJPEG) {
        stChnAttr.stRcAttr.stMjpegCbr.u32BitRate = pIc->chgBitrate = chgBitrate;
        stChnAttr.stRcAttr.stMjpegCbr.fr32DstFrameRate = pIc->chgFramerate =
            chgFramerate;
    }

    CVI_VENC_SetChnAttr(VencChn, &stChnAttr);

    return s32Ret;
}