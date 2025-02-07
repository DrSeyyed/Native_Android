#ifndef AV_DECODER_H
#define AV_DECODER_H

#include <chrono>
#include <memory>
#include <queue>
#include <android/asset_manager.h>
extern "C" {
#include <libavformat/avformat.h>
#include <libavutil/channel_layout.h>
#include <libavutil/error.h>
#include <libavcodec/avcodec.h>
#include <libswscale/swscale.h>
#include <libswresample/swresample.h>
#include <sys/types.h>
#include <unistd.h>
#include "libavutil/imgutils.h"
#include <libavutil/opt.h>
#include <libavutil/hwcontext.h>
};

class AVDecoder{
private:
    int video_stream_index,audio_stream_index;
    std::chrono::system_clock::time_point start_time;
    std::chrono::system_clock::time_point pause_time;
    double delay_time=0;
    int video_frame_count = 0;
    int audio_frame_count = 0;
    AVFormatContext* v_format_ctx;
    AVFormatContext* a_format_ctx;
    AVCodecContext* v_codec_ctx;
    AVCodecContext* a_codec_ctx;
    AAssetManager* asset_manager;
    const int maxVideoBufferSize = 3;
    const int maxAudioBufferSize = 10;
    std::atomic<bool> stopFlag=false;
    std::thread audio_thread,video_thread;
    std::queue<std::pair<AVFrame*,double>> videoFrameQueue,audioFrameQueue;
    std::mutex videoQueueMutex,audioQueueMutex;
    std::condition_variable videoQueueCondVar, audioQueueCondVar;

    static int readFunction(void *opaque, uint8_t *buf, int buf_size) {
        AAsset *asset = static_cast<AAsset*>(opaque);
        int bytesRead = AAsset_read(asset, buf, buf_size);
        if (bytesRead == 0) {
            // Simulate EOF
            return AVERROR_EOF;
        }
        return bytesRead;
    }
    void closeAsset(AVFormatContext *formatCtx) {
        if (formatCtx) {
            if (formatCtx->pb) {
                AAsset *asset = (AAsset *)formatCtx->pb->opaque;
                avio_context_free(&formatCtx->pb);
                AAsset_close(asset);
            }
            avformat_close_input(&formatCtx);
        }
    }
    static int64_t seekFunction(void *opaque, int64_t offset, int whence) {
        AAsset *asset = static_cast<AAsset*>(opaque);
        off_t newPos = AAsset_seek(asset, offset, whence);
        if (newPos == -1) {
            return AVERROR(EIO);  // Error during seek
        }
        return newPos;
    }
    AVFormatContext* openAsset(const char *filename) {
        AAsset *asset = AAssetManager_open(asset_manager, filename, AASSET_MODE_UNKNOWN);
        if (!asset) {
            fprintf(stderr, "Could not open asset %s\n", filename);
            return nullptr;
        }

        // Create a custom AVIOContext
        uint8_t *buffer = (uint8_t *)av_malloc(8192);
        if (!buffer) {
            fprintf(stderr, "Could not allocate buffer\n");
            return nullptr;
        }

        AVIOContext *avio = avio_alloc_context(buffer, 8192, 0, asset, readFunction, nullptr, seekFunction);
        if (!avio) {
            fprintf(stderr, "Could not allocate AVIOContext\n");
            av_free(buffer);
            return nullptr;
        }

        AVFormatContext *formatCtx = avformat_alloc_context();
        if (!formatCtx) {
            fprintf(stderr, "Could not allocate AVFormatContext\n");
            avio_context_free(&avio);
            return nullptr;
        }

        formatCtx->pb = avio;
        formatCtx->flags |= AVFMT_FLAG_CUSTOM_IO;
        if (avformat_open_input(&formatCtx, nullptr, nullptr, nullptr) < 0) {
            fprintf(stderr, "Could not open input\n");
            avformat_free_context(formatCtx);
            avio_context_free(&avio);
            return nullptr;
        }

        return formatCtx;
    }
    static enum AVHWDeviceType find_hw_device_type_by_name(const char *name) {
        enum AVHWDeviceType type = av_hwdevice_find_type_by_name(name);
        if (type == AV_HWDEVICE_TYPE_NONE) {
            printf("Device type '%s' is not supported.\n", name);
        }
        return type;
    }
    static AVBufferRef* init_hw_device(enum AVHWDeviceType type) {
        AVBufferRef *hw_device_ctx = NULL;
        if (av_hwdevice_ctx_create(&hw_device_ctx, type, NULL, NULL, 0) < 0) {
            printf("Failed to create hardware device context for type %d.\n", type);
            return NULL;
        }
        return hw_device_ctx;
    }
    AVCodecContext* initializeCodecContext(AVFormatContext* formatContext, int& streamIndex, AVMediaType type) {
        for (unsigned int i = 0; i < formatContext->nb_streams; i++) {
            if (formatContext->streams[i]->codecpar->codec_type == type) {
                streamIndex = i;
                AVCodecParameters* codecParameters = formatContext->streams[i]->codecpar;
                const AVCodec* codec = avcodec_find_decoder(codecParameters->codec_id);
                if (!codec) {
                    throw std::runtime_error("Unsupported codec");
                }
                if(codec->id == AV_CODEC_ID_VP9) {
                    codec = avcodec_find_decoder_by_name("libvpx-vp9");
                }
                AVCodecContext* codecContext = avcodec_alloc_context3(codec);
                if (!codecContext) {
                    throw std::runtime_error("Could not allocate codec context");
                }
                if (avcodec_parameters_to_context(codecContext, codecParameters) < 0) {
                    throw std::runtime_error("Could not copy codec parameters to context");
                }
                const char* hwaccels[] = { "mediacodec", "cuda", "vaapi", "dxva2", "videotoolbox" };
                for (int i = 0; i < sizeof(hwaccels) / sizeof(hwaccels[0]); i++) {
                    enum AVHWDeviceType hw_type = find_hw_device_type_by_name(hwaccels[i]);
                    if (hw_type != AV_HWDEVICE_TYPE_NONE) {
                        AVBufferRef *hw_device_ctx = init_hw_device(hw_type);
                        if (hw_device_ctx) {
                            codecContext->hw_device_ctx = av_buffer_ref(hw_device_ctx);
                            aout<<"Hardware decoder "<<hwaccels[i]<<" is started"<<std::endl;
                            break;
                        }
                    }
                }
                if (avcodec_open2(codecContext, codec, nullptr) < 0) {
                    throw std::runtime_error("Could not open codec");
                }
                codecContext->thread_count = 8;
                codecContext->flags |= AV_CODEC_FLAG_LOW_DELAY;
                return codecContext;
            }
        }
        return nullptr;
    }
    SwsContext* getSwsContext(AVFrame* frame,int width, int height){
        SwsContext* swsContext =sws_getContext(
                frame->width, frame->height, (AVPixelFormat)frame->format,
                width, height, AV_PIX_FMT_RGBA,
                SWS_BILINEAR, nullptr, nullptr, nullptr);
        if(!swsContext){
            throw std::runtime_error("cant initialize sws video");
        }
        return swsContext;
    }
    SwrContext* getSwrContext(AVFrame* frame,int64_t out_channel_layout,AVSampleFormat out_sample_fmt,int out_sample_rate) {
        SwrContext *swrContext {swr_alloc_set_opts(nullptr,out_channel_layout,out_sample_fmt,out_sample_rate,frame->channel_layout,(AVSampleFormat)frame->format,frame->sample_rate,0,nullptr)};
        if (swr_init(swrContext) < 0) {
            throw std::runtime_error("Could not initialize swr context");
        }
        return swrContext;
    }



public:
    AVDecoder(){
        video_stream_index=-1;
        audio_stream_index=-1;
        start_time=std::chrono::system_clock::now();
        v_format_ctx= nullptr;
        a_format_ctx= nullptr;
        v_codec_ctx = nullptr;
        a_codec_ctx = nullptr;
    }
    ~AVDecoder(){
        stopFlag=true;
        videoQueueCondVar.notify_one();
        audioQueueCondVar.notify_one();
        if(video_thread.joinable())
            video_thread.join();
        if(audio_thread.joinable())
            audio_thread.join();
        for(int i=0;i<videoFrameQueue.size();i++) {
            av_frame_free(&videoFrameQueue.front().first);
            videoFrameQueue.pop();
        }
        for(int i=0;i<audioFrameQueue.size();i++) {
            av_frame_free(&audioFrameQueue.front().first);
            audioFrameQueue.pop();
        }
        avcodec_free_context(&v_codec_ctx);
        avcodec_free_context(&a_codec_ctx);
        closeAsset(v_format_ctx);
        closeAsset(a_format_ctx);
    }
    void set_asset_manager(AAssetManager* a_m){
        asset_manager=a_m;
    }
    void av_open(const char *filename) {
        v_format_ctx = openAsset(filename);
        a_format_ctx = openAsset(filename);
        video_stream_index=-1;
        audio_stream_index=-1;
        v_codec_ctx = initializeCodecContext(v_format_ctx,video_stream_index,AVMEDIA_TYPE_VIDEO);
        a_codec_ctx = initializeCodecContext(a_format_ctx,audio_stream_index,AVMEDIA_TYPE_AUDIO);
        start_time = std::chrono::system_clock::now();
        if(video_stream_index==-1 and audio_stream_index==-1){
            throw std::runtime_error("Cant! find AVStream");
        }
    }
    void get_video_buffer(uint8_t* &frame_buffer,int width,int height,double &pts,bool &success,std::atomic<bool> &finished){
        success=false;
        finished=false;
        auto frame = av_frame_alloc();
        auto packet = av_packet_alloc();
        if(!frame || !packet){
            throw std::runtime_error("cant alloc AVFrame or AVPacket!");
        }
        while (true) {
            if (av_read_frame(v_format_ctx, packet) < 0) {
                break;  // End of media
            }
            if (packet->stream_index == video_stream_index) {
                if (avcodec_send_packet(v_codec_ctx, packet) == 0) {
                    if (avcodec_receive_frame(v_codec_ctx, frame) == 0) {
                        SwsContext* swsContext= getSwsContext(frame,width,height);
                        int frame_buffer_size = av_image_get_buffer_size(AV_PIX_FMT_RGBA, width, height, 1);
                        frame_buffer = (uint8_t*)av_malloc(frame_buffer_size+1);
                        uint8_t* dest[4] = { frame_buffer, nullptr, nullptr, nullptr };
                        int dest_line_size[4] = { 4 * width, 0, 0, 0 };
                        sws_scale(swsContext, frame->data, frame->linesize, 0, frame->height, dest, dest_line_size);
                        video_frame_count++;
                        if (frame->pts == AV_NOPTS_VALUE) {
                            pts = video_frame_count * av_q2d(av_inv_q(v_codec_ctx->framerate)) * av_q2d(v_format_ctx->streams[video_stream_index]->time_base);
                        }
                        else
                            pts = frame->pts * av_q2d(v_format_ctx->streams[video_stream_index]->time_base);
                        success=true;
                        sws_freeContext(swsContext);
                        break;
                    }
                }
            }
            av_packet_unref(packet);
        }
        av_frame_free(&frame);
        av_packet_free(&packet);
        finished=true;
    }
    void get_video_frame(AVFrame* &frame,double &pts,bool &success,std::atomic<bool> &finished){
        success=false;
        finished=false;
        frame = av_frame_alloc();
        auto packet = av_packet_alloc();
        if(!frame || !packet){
            throw std::runtime_error("cant alloc AVFrame or AVPacket!");
        }
        while (true) {
            if (av_read_frame(v_format_ctx, packet) < 0) {
                break;  // End of media
            }
            if (packet->stream_index == video_stream_index) {
                if (avcodec_send_packet(v_codec_ctx, packet) == 0) {
                    if (avcodec_receive_frame(v_codec_ctx, frame) == 0) {
                        video_frame_count++;
                        if (frame->pts == AV_NOPTS_VALUE) {
                            pts = video_frame_count * av_q2d(av_inv_q(v_codec_ctx->framerate)) * av_q2d(v_format_ctx->streams[video_stream_index]->time_base);
                        }
                        else
                            pts = frame->pts * av_q2d(v_format_ctx->streams[video_stream_index]->time_base);
                        success=true;
                        break;
                    }
                }
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        finished=true;
    }
    void get_audio_buffer(uint8_t* &frame_buffer,int& buffer_size,int64_t out_channel_layout,AVSampleFormat out_sample_fmt,int out_sample_rate,bool &success,std::atomic<bool> &finished){
        success=false;
        finished=false;
        auto frame = av_frame_alloc();
        auto packet = av_packet_alloc();
        if(!frame || !packet){
            throw std::runtime_error("cant alloc AVFrame or AVPacket!");
        }
        while (true) {
            if (av_read_frame(a_format_ctx, packet) < 0) {
                break;  // End of media
            }
            if (packet->stream_index == audio_stream_index) {
                if (avcodec_send_packet(a_codec_ctx, packet) == 0) {
                    if (avcodec_receive_frame(a_codec_ctx, frame) == 0) {
                        SwrContext* swrContext= getSwrContext(frame,out_channel_layout,out_sample_fmt,out_sample_rate);
                        int out_nb_samples = av_rescale_rnd(frame->nb_samples, out_sample_rate, frame->sample_rate, AV_ROUND_UP);
                        av_samples_alloc(&frame_buffer, nullptr, out_channel_layout, out_nb_samples, out_sample_fmt, 0);
                        buffer_size = swr_convert(swrContext, &frame_buffer, out_nb_samples,(const uint8_t **)(frame->data), frame->nb_samples);
                        if(buffer_size<0 )
                            throw std::runtime_error("Swr conversion error");
                        success=true;
                        swr_free(&swrContext);
                        break;
                    }
                }
            }
            av_packet_unref(packet);
        }
        av_frame_free(&frame);
        av_packet_free(&packet);
        finished=true;
    }
    void get_audio_frame(AVFrame* &frame,bool &success,std::atomic<bool> &finished){
        success=false;
        finished=false;
        frame = av_frame_alloc();
        auto packet = av_packet_alloc();
        if(!frame || !packet){
            throw std::runtime_error("cant alloc AVFrame or AVPacket!");
        }
        while (true) {
            if (av_read_frame(a_format_ctx, packet) < 0) {
                break;  // End of media
            }
            if (packet->stream_index == audio_stream_index) {
                if (avcodec_send_packet(a_codec_ctx, packet) == 0) {
                    if (avcodec_receive_frame(a_codec_ctx, frame) == 0) {
                        success=true;
                        break;
                    }
                }
            }
            av_packet_unref(packet);
        }
        av_packet_free(&packet);
        finished=true;
    }
    void start_video_decoding() {
        while (!stopFlag) {
            AVFrame* video_frame;
            double pts=0;
            bool video_success=false;
            std::atomic<bool> video_finished(false);
            get_video_frame(video_frame, pts, video_success,video_finished);
            if(!video_success) {
                std::unique_lock<std::mutex> lock(videoQueueMutex);
                videoQueueCondVar.wait(lock, [this]{ return videoFrameQueue.size() < maxVideoBufferSize or stopFlag; });
                videoFrameQueue.push(std::pair(nullptr,0));
                videoQueueCondVar.notify_one();
                return;
            }
            std::unique_lock<std::mutex> lock_v(videoQueueMutex);
            videoQueueCondVar.wait(lock_v, [this]{ return videoFrameQueue.size() < maxVideoBufferSize or stopFlag; });
            videoFrameQueue.push(std::pair(video_frame,pts));
            videoQueueCondVar.notify_one();
            if(stopFlag)
                break;
        }
    }
    void start_audio_decoding() {
        while (!stopFlag) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            get_audio_frame(audio_frame,audio_success,audio_finished);
            if(!audio_success) {
                std::unique_lock<std::mutex> lock(audioQueueMutex);
                audioQueueCondVar.wait(lock, [this]{ return audioFrameQueue.size() < maxAudioBufferSize or stopFlag; });
                audioFrameQueue.push(std::pair(nullptr,0));
                audioQueueCondVar.notify_one();
                return;
            }
            std::unique_lock<std::mutex> lock_a(audioQueueMutex);
            audioQueueCondVar.wait(lock_a, [this]{ return audioFrameQueue.size() < maxAudioBufferSize or stopFlag; });
            audioFrameQueue.push(std::pair(audio_frame,0));
            audioQueueCondVar.notify_one();
            if(stopFlag)
                break;
        }
    }
    void start_thread(){
        video_thread = std::thread([this](){ start_video_decoding(); });
        audio_thread = std::thread([this](){ start_audio_decoding(); });
    }
    std::pair<AVFrame*,double> getVideoFrame(){
        std::unique_lock<std::mutex> lock_v(videoQueueMutex);
        videoQueueCondVar.wait(lock_v, [this] { return !videoFrameQueue.empty(); });
        auto video_frame = videoFrameQueue.front();
        videoFrameQueue.pop();
        videoQueueCondVar.notify_one();
        return video_frame;
    }
    std::pair<AVFrame*,double> getAudioFrame(){
        std::unique_lock<std::mutex> lock_a(audioQueueMutex);
        audioQueueCondVar.wait(lock_a, [this] { return !audioFrameQueue.empty(); });
        auto audio_frame = audioFrameQueue.front();
        audioFrameQueue.pop();
        audioQueueCondVar.notify_one();
        return audio_frame;
    }
    void pause(){
        pause_time = std::chrono::system_clock::now();
    }
    void resume(){
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = now - pause_time;
        delay_time+=diff.count();
    }
    double get_duration(){
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = now - start_time;
        return diff.count()-delay_time;
    }
    bool is_reach(double pts){
        return pts <= get_duration();
    }
};

#endif
