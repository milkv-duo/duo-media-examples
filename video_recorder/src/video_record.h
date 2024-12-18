#pragma once

extern "C" {
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/opt.h"
}

#include "cvi_sdk.h"
#include "log.h"

#include "Poco/Runnable.h"
#include "Poco/Thread.h"
#include "Poco/Timer.h"

struct RecordConf {
    std::string _format = "flv";
    int _record_time = 60; // seconds
    std::string _out_file = "out.flv";
};

class VideoRecord : public Poco::Runnable, public EncoderCallback {
  public:
    VideoRecord();
    ~VideoRecord();

    int32_t initVideoEncoder();
    int32_t initFfmpeg(RecordConf *pConf);
    void stop() { _running = false; }

    //Poco::Runnable
    //Do whatever the thread needs to do.
    void run();

    //EncoderCallback
    void onVideoData(AVPacket *pkt) override;

  private:
    void onTimer(Poco::Timer &timer);

  private:
    bool _running = true;

    Poco::Timer _timer;
    CviVideoEncoder *_encoder = nullptr;
    AVFormatContext *_fmt_ctx = nullptr;
    AVOutputFormat *_out_fmt = nullptr;
    AVStream *_video_stream = nullptr;

    int64_t _first_pts_time = 0;
    int64_t _last_pts_time = 0;
};