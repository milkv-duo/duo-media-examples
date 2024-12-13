#include "video_record.h"

VideoRecord::VideoRecord() {}

VideoRecord::~VideoRecord() {
    if (_encoder) {
        delete _encoder;
    }

    if (_fmt_ctx) {
        avio_closep(&_fmt_ctx->pb);
        avformat_free_context(_fmt_ctx);
    }
}

int32_t VideoRecord::initVideoEncoder() {
    if (_encoder == NULL) {
        _encoder = new CviVideoEncoder();
        struct VideoInfo videoInfo;
        int32_t ret = _encoder->init_cfg("264", &videoInfo);

        if (ret != 0) {
            ERROR("init_cfg failed! ret = {0}", ret);
            return ret;
        }
    }
    _encoder->venc_start();
    return 0;
}

int32_t VideoRecord::initFfmpeg(RecordConf *pConf) {
    std::string format = "flv";
    if (pConf->_format == "mp4") {
        format = "mp4";
    }

    _out_fmt = av_guess_format(format.c_str(), pConf->_out_file.c_str(), NULL);
    if (!_out_fmt) {
        ERROR("Could not deduce output format from file extension!");
        return -1;
    }

    avformat_alloc_output_context2(&_fmt_ctx, _out_fmt, NULL,
                                   pConf->_out_file.c_str());
    if (!_fmt_ctx) {
        ERROR("Failed to create output context!");
        return -1;
    }

    // 创建视频流
    _video_stream = avformat_new_stream(_fmt_ctx, NULL);
    if (!_video_stream) {
        ERROR("Failed to allocate video stream");
        return -1;
    }

    // 设置编码器
    AVCodecParameters *codecParameters = _video_stream->codecpar;
    codecParameters->codec_id = AV_CODEC_ID_H264;
    codecParameters->codec_type = AVMEDIA_TYPE_VIDEO;
    codecParameters->width = 1920;  // 设定视频宽度
    codecParameters->height = 1080; // 设定视频高度
    codecParameters->format = AV_PIX_FMT_NV21;

    // 打开输出文件
    if (avio_open(&_fmt_ctx->pb, pConf->_out_file.c_str(), AVIO_FLAG_WRITE) <
        0) {
        ERROR("Could not open output file: {0}", pConf->_out_file);
        return -1;
    }

    // 写文件头
    int ret = avformat_write_header(_fmt_ctx, NULL);
    if (ret != 0) {
        ERROR("Could not write header: {0}", ret);
        return -1;
    }

    _timer.setStartInterval(pConf->_record_time * 1000);
    return 0;
}

void VideoRecord::onVideoData(AVPacket *pkt) {
    if (_first_pts_time == 0) {
        _first_pts_time = _last_pts_time = pkt->pts;
    }

    pkt->duration = pkt->pts - _last_pts_time;
    _last_pts_time = pkt->pts;

    av_packet_rescale_ts(pkt, AV_TIME_BASE_Q, _video_stream->time_base);

    if (av_interleaved_write_frame(_fmt_ctx, pkt) < 0) {
        ERROR("Error while writing frame!");
        return;
    }
}

void VideoRecord::run() {
    _timer.start(
        Poco::TimerCallback<VideoRecord>(*this, &VideoRecord::onTimer));
    int ret = 0;
    while (_running) {
        ret = _encoder->getStream(this);
        if (ret != 0) {
            WARN("get avpacket failed! ret = {0}", ret);
            Poco::Thread::sleep(100);
            continue;
        }
    }

    _video_stream->duration =
        av_rescale_q(_last_pts_time - _first_pts_time, AV_TIME_BASE_Q,
                     _video_stream->time_base);
    _fmt_ctx->duration = (_last_pts_time - _first_pts_time)/1000;
    av_write_trailer(_fmt_ctx);
}

void VideoRecord::onTimer(Poco::Timer &timer) {
    // TODO
    stop();
}