#ifndef SUBPAGE_H
#define SUBPAGE_H

#include "Page.h"

bool correct= false;
std::vector<Model> last_model;

class SubPage0 : public Page{
public:
    SubPage0(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        if(!media_decoder.openFile(app_->activity->assetManager,"bazijenab.mp4"))
            throw std::runtime_error("cant open video file!");
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
    }
    std::vector <Model> getModels(){
        std::vector <Model> tmp;
        uint8_t* video_buffer=nullptr;
        int width=0,height=0;
        aout<<"here iam 0"<<std::endl;
        if(media_decoder.decodePureFrame(video_buffer,width,height)) {
            if(video_buffer!= nullptr) {
                int yPlaneSize = width * height;
                int uvPlaneSize = (width / 2) * (height / 2);
                uint8_t* tmpor =new uint8_t[width*height];
                for(int i=0;i<width*height;i++)
                    tmpor[i]=255;
                auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_buffer, width, height);
                auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                        video_buffer + yPlaneSize, width/2, height/2);
                auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                        video_buffer + yPlaneSize + uvPlaneSize, width/2, height/2);
                auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, width, height);
                tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                       u_texture_ptr,
                                                       v_texture_ptr,
                                                       a_texture_ptr,
                                                       -half_width, +half_width, -half_height,
                                                       +half_height));
                delete[] tmpor;
                delete[] video_buffer;
            }
        }
        aout<<"here iam 1"<<std::endl;
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
private:
    MediaCodecDecoder media_decoder;
};

class SubPage1 : public Page{
public:
    SubPage1(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("bazijenab.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_done=false;
        is_initiate=true;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=1;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;

            uint8_t tmp_alpha[1];
            tmp_alpha[0] = 255;

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], width, height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], width / 2, height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], width / 2, height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmp_alpha, 1, 1);
            tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
};
class SubPage2 : public Page{
public:
    SubPage2(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("malignant.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        uint8_t* video_buffer = nullptr;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_buffer,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_buffer(video_buffer, 720, 1080, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            int buffer_size=0;
            uint8_t* audio_buffer = nullptr;
            std::thread th2([a = av_decoder, &audio_buffer, &buffer_size, &audio_success,&audio_finished]() {
                a->get_audio_buffer(audio_buffer, buffer_size, AV_CH_LAYOUT_MONO, AV_SAMPLE_FMT_S16,
                                    44100, audio_success,audio_finished);
            });
            th2.join();
            if(audio_success)
                audio_player->appendBuffer(audio_id,reinterpret_cast<float *>(audio_buffer),buffer_size);
            av_free(audio_buffer);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            return tmp;
        }
        auto texture_ptr = TextureAsset::makeAssetFromData(video_buffer,720,1080);
        tmp.emplace_back(Model::createModel(texture_ptr, -half_width,+half_width,-half_height,+half_height));
        av_free(video_buffer);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
};
class SubPage3:  public Page{
public:
    SubPage3(android_app* pApp): Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        auto assetManager = app_->activity->assetManager;
        text_poster = TextureAsset::loadAsset(assetManager, "poster.png");
        is_initiate=true;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        tmp.emplace_back(Model::createModel(text_poster, -half_width,+half_width,-half_height,+half_height));
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{
        for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
            auto &motionEvent = inputBuffer->motionEvents[i];
            auto action = motionEvent.action;
            auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            auto &pointer = motionEvent.pointers[pointerIndex];
            auto x = GameActivityPointerAxes_getX(&pointer);
            auto y = GameActivityPointerAxes_getY(&pointer);
            if((action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN){
                checkStartClick(x,y);
            }
        }
    }
private:
    void checkStartClick(int x, int y){
        float centerx=width_/2;float centery=height_/2;float step=height_/(half_height*2);
        float realx= (x-centerx)/step;float realy= (centery-y)/step;
        if(std::abs(realx)<0.6*half_width and std::abs(realy+1.55)<0.95)
        {
            is_done=true;
        }
    }
    std::shared_ptr<TextureAsset> text_poster;
};
class SubPage4 : public Page{
public:
    SubPage4(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("malignant.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], width, height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], width / 2, height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], width / 2, height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, 1, 1);
            tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
};
class SubPage5 : public Page{
public:
    SubPage5(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("film.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], width, height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], width / 2, height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], width / 2, height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, 1, 1);
            tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
};
class SubPage6 : public Page{
public:
    SubPage6(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("scorepage_x265.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], width, height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], width / 2, height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], width / 2, height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, 1, 1);
            tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
};
class SubPage7 : public Page{
public:
    SubPage7(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        auto assetManager = app_->activity->assetManager;
        text_start_button = TextureAsset::loadAsset(assetManager, "start_button.png");
        text_cast_button = TextureAsset::loadAsset(assetManager, "cast_button.png");
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("startpage_x265.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;

    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], width, height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], width / 2, height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], width / 2, height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, 1, 1);
            tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        tmp.emplace_back(Model::createModel(text_start_button, -half_width,+half_width,0,+half_height/2));
        tmp.emplace_back(Model::createModel(text_cast_button, -half_width,+half_width,-half_height/2,0));
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{
        for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
            auto &motionEvent = inputBuffer->motionEvents[i];
            auto action = motionEvent.action;
            auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            auto &pointer = motionEvent.pointers[pointerIndex];
            auto x = GameActivityPointerAxes_getX(&pointer);
            auto y = GameActivityPointerAxes_getY(&pointer);
            if((action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN){
                checkStartClick(x,y);
                checkCastClick(x,y);
            }
        }
    }
private:
    void checkStartClick(int x, int y){
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,-half_width,+half_width) and isBetween(loc.second,0,+half_height/2))
        {
            is_done=true;
            next_page=3;
        }
    }
    void checkCastClick(int x, int y){
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,-half_width,+half_width) and isBetween(loc.second,-half_height/2,0))
        {
            is_done=true;
            next_page=7;
        }
    }
    std::shared_ptr<TextureAsset> text_start_button;
    std::shared_ptr<TextureAsset> text_cast_button;
};
class SubPage8 : public Page{
public:
    SubPage8(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        still_image=last_model;
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("newyork1_x265.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;

    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame=video_frame_with_pts.first;
        auto pts=video_frame_with_pts.second;
        while(pts > av_decoder->get_duration()) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t tmpor[1] = {255};
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], video_frame->linesize[1], height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], video_frame->linesize[2], height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, 1, 1);

            model.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                              u_texture_ptr,
                                                              v_texture_ptr,
                                                              a_texture_ptr,
                                                              -half_width, +half_width, -half_height,
                                                              +half_height));
        }
        av_frame_free(&video_frame);
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
    std::vector<Model> still_image;
};
class SubPage9 : public Page{
public:
    SubPage9(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        auto assetManager = app_->activity->assetManager;
        text_level1_logos = TextureAsset::loadAsset(assetManager, "level1_logos.png");
        text_check_button = TextureAsset::loadAsset(assetManager, "check_button.png");
        text_brand_question = TextureAsset::loadAsset(assetManager, "brand_question.png");
        uint8_t selector_temp[4]={0,255,0,120};
        text_selector = TextureAsset::makeAssetFromData(selector_temp,1,1);
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("answerpage_x265.mp4");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        memset(checked, 0, sizeof(checked[0][0]) * 3 * 4);
        is_initiate=true;
        is_done=false;

    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=4;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], width, height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], width / 2, height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], width / 2, height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(tmpor, 1, 1);
            tmp.emplace_back(Model::createYUVModel(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        tmp.emplace_back(Model::createModel(text_brand_question, -half_width,+half_width,+half_height*0.6,+half_height));
        tmp.emplace_back(Model::createModel(text_level1_logos, -half_width*0.8,+half_width*0.4,-half_height*0.9,half_height*0.6));
        tmp.emplace_back(Model::createModel(text_check_button, +half_width*0.65,half_width*0.9,-half_height*0.5,0));
        for(int i=0;i<3;i++)
            for(int j=0;j<4;j++)
                if(checked[i][j])
                    tmp.emplace_back(Model::createModel(text_selector, -half_width*0.8+j*half_width*0.3,-half_width*0.8+(j+1)*half_width*0.3
                                                        ,-half_height*0.9+i*half_height*0.5,-half_height*0.9+(i+1)*half_height*0.5));
        last_model=tmp;
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{
        for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
            auto &motionEvent = inputBuffer->motionEvents[i];
            auto action = motionEvent.action;
            auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            auto &pointer = motionEvent.pointers[pointerIndex];
            auto x = GameActivityPointerAxes_getX(&pointer);
            auto y = GameActivityPointerAxes_getY(&pointer);
            if((action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN){
                checkCheckClick(x,y);
                checkSelectClick(x,y);
            }
        }
    }
private:
    void checkCheckClick(int x, int y){
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,+half_width*0.65,half_width*0.9) and isBetween(loc.second,-half_height*0.5,0))
        {
            is_done=true;
            for(int i=0;i<3;i++)
                for(int j=0;j<4;j++)
                    if(answered[i][j]!=checked[i][j]){
                        next_page=6;
                        return;
                    }
            next_page=5;
        }
    }
    void checkSelectClick(int x, int y){
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,-half_width*0.8,+half_width*0.4) and isBetween(loc.second,-half_height*0.9,half_height*0.6))
        {
            int j = (loc.first- (-half_width*0.8))/(half_width*0.3);
            int i = (loc.second- (-half_height*0.9))/(half_height*0.5);
            checked[i][j]=1-checked[i][j];
        }
    }
    std::shared_ptr<TextureAsset> text_level1_logos;
    std::shared_ptr<TextureAsset> text_selector;
    std::shared_ptr<TextureAsset> text_brand_question;
    std::shared_ptr<TextureAsset> text_check_button;
    bool checked[3][4];
    bool answered[3][4]={
            {0,1,0,0},
            {0,0,0,0},
            {1,0,1,0}
    };
};
class SubPage10 : public Page{
public:
    SubPage10(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        still_image=last_model;
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("celeb_fix.mkv");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp=still_image;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], video_frame->linesize[1], height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], video_frame->linesize[2], height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], height);

            tmp.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                   u_texture_ptr,
                                                   v_texture_ptr,
                                                   a_texture_ptr,
                                                   (float)width/video_frame->linesize[0],
                                                   (float)width/video_frame->linesize[1]/2,
                                                   (float)width/video_frame->linesize[2]/2,
                                                   (float)width/video_frame->linesize[3],
                                                   -half_width, +half_width, -half_height,
                                                   +half_height));
        }
        av_frame_free(&video_frame);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
    std::vector<Model> still_image;
};
class SubPage11 : public Page{
public:
    SubPage11(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = nullptr;
        audio_player = std::make_shared<AudioPlayer>();
        still_image=last_model;
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("wrong_fix.mkv");
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;

    }
    std::vector <Model> getModels() override{
        std::vector <Model> tmp=still_image;
        AVFrame* video_frame;
        double pts=0;
        bool video_success=false;
        std::atomic<bool> video_finished(false);
        std::thread th1([a = av_decoder, &video_frame,&pts,&video_success,&video_finished]()
                        {
                            a->get_video_frame(video_frame, pts, video_success,video_finished);
                        });
        while(!video_finished.load() or pts > av_decoder->get_duration()) {
            std::atomic<bool> audio_finished(false);
            bool audio_success=false;
            AVFrame* audio_frame;
            std::thread th2([a = av_decoder,&audio_frame, &audio_success,&audio_finished]() {
                a->get_audio_frame(audio_frame,audio_success,audio_finished);
            });
            th2.join();
            if(audio_success) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                          reinterpret_cast<float *>(audio_frame->data[0]),
                                          buffer_size);
            }
            av_frame_free(&audio_frame);
        }
        th1.join();
        if(!video_success){
            aout<<"cant read frame"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {
            int width = video_frame->width;
            int height = video_frame->height;
            uint8_t *tmpor = new uint8_t[1];
            tmpor[0] = 255;
            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[1], video_frame->linesize[1], height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(
                    video_frame->data[2], video_frame->linesize[2], height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], height);

            tmp.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                              u_texture_ptr,
                                                              v_texture_ptr,
                                                              a_texture_ptr,
                                                              (float)width/video_frame->linesize[0],
                                                              (float)width/video_frame->linesize[1]/2,
                                                              (float)width/video_frame->linesize[2]/2,
                                                              (float)width/video_frame->linesize[3],
                                                              -half_width, +half_width, -half_height,
                                                              +half_height));
        }
        av_frame_free(&video_frame);
        return tmp;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
    std::vector<Model> still_image;
};


class SubPageNEW : public Page{
public:
    SubPageNEW(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("newyork1_x265.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], video_frame->height);
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float)video_frame->width/video_frame->linesize[0],
                                                                (float)video_frame->width/video_frame->linesize[1]/2,
                                                                (float)video_frame->width/video_frame->linesize[2]/2,
                                                                (float)video_frame->width/video_frame->linesize[3],
                                                                -half_width, +half_width, -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
};


class SubPage001 : public Page{
public:
    SubPage001(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("bazijenab.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
        still_image=last_model;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=1;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3],video_frame->linesize[3],video_frame->height);
            if(video_frame->linesize[3]==0){
                uint8_t temp_a[1] = {255};
                video_frame->linesize[3]=video_frame->width;
                a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
            }
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float) video_frame->width /
                                                                video_frame->linesize[0],
                                                                (float) video_frame->width /
                                                                video_frame->linesize[1] / 2,
                                                                (float) video_frame->width /
                                                                video_frame->linesize[2] / 2,
                                                                (float) video_frame->width /
                                                                video_frame->linesize[3],
                                                                -half_width, +half_width,
                                                                -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        if(is_done==false)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
private:
    std::vector<Model> still_image;
};
class SubPage002 : public Page{
public:
    SubPage002(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("malignant.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
        still_image=last_model;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], video_frame->height);
            if(video_frame->linesize[3]==0){
                uint8_t temp_a[1] = {255};
                video_frame->linesize[3]=video_frame->width;
                a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
            }
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float)video_frame->width/video_frame->linesize[0],
                                                                (float)video_frame->width/video_frame->linesize[1]/2,
                                                                (float)video_frame->width/video_frame->linesize[2]/2,
                                                                (float)video_frame->width/video_frame->linesize[3],
                                                                -half_width, +half_width, -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        if(is_done==false)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
private:
    std::vector<Model> still_image;
};
class SubPage003 : public Page{
public:
    SubPage003(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("page_start_x265.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        text_start_button = TextureAsset::loadAsset(app_->activity->assetManager, "start2.png");
        text_cast_button = TextureAsset::loadAsset(app_->activity->assetManager, "cast2.png");
        is_initiate=true;
        is_done=false;
        still_image=last_model;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], video_frame->height);
            if(video_frame->linesize[3]==0){
                uint8_t temp_a[1] = {255};
                video_frame->linesize[3]=video_frame->width;
                a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
            }
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float)video_frame->width/video_frame->linesize[0],
                                                                (float)video_frame->width/video_frame->linesize[1]/2,
                                                                (float)video_frame->width/video_frame->linesize[2]/2,
                                                                (float)video_frame->width/video_frame->linesize[3],
                                                                -half_width, +half_width, -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        model.emplace_back(Model::createModel(text_start_button, -half_width,+half_width,0,+half_height/2));
        model.emplace_back(Model::createModel(text_cast_button, -half_width,+half_width,-half_height/2,0));

        if(is_done==false)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{
        for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
            auto &motionEvent = inputBuffer->motionEvents[i];
            auto action = motionEvent.action;
            auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            auto &pointer = motionEvent.pointers[pointerIndex];
            auto x = GameActivityPointerAxes_getX(&pointer);
            auto y = GameActivityPointerAxes_getY(&pointer);
            if((action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN){
                checkStartClick(x,y);
                checkCastClick(x,y);
            }
        }
    }
private:
    void checkStartClick(int x, int y){
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,-half_width,+half_width) and isBetween(loc.second,0,+half_height/2))
        {
            is_done=true;
            next_page=3;
        }
    }
    void checkCastClick(int x, int y){
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,-half_width,+half_width) and isBetween(loc.second,-half_height/2,0))
        {
            is_done=true;
            next_page=6;
        }
    }
    std::vector<Model> still_image;
    std::shared_ptr<TextureAsset> text_start_button;
    std::shared_ptr<TextureAsset> text_cast_button;
};
class SubPage004 : public Page{
public:
    SubPage004(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        is_asked=is_answered=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("goal_001_x265.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        text_level1_logos = TextureAsset::loadAsset(app_->activity->assetManager, "result_ask.png");
        text_check_button = TextureAsset::loadAsset(app_->activity->assetManager, "check_button.png");
        text_brand_question = TextureAsset::loadAsset(app_->activity->assetManager, "question.png");
        uint8_t selector_temp[4]={0,255,0,120};
        text_selector = TextureAsset::makeAssetFromData(selector_temp,1,1);
        memset(checked,0,4);
        is_initiate=true;
        is_done=false;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        if(!is_asked or is_answered) {
            auto video_frame_with_pts = av_decoder->getVideoFrame();
            auto video_frame = video_frame_with_pts.first;
            auto pts = video_frame_with_pts.second;
            while (!av_decoder->is_reach(pts)) {
                if (audio_done)
                    continue;
                auto audio_frame_with_pts = av_decoder->getAudioFrame();
                auto audio_frame = audio_frame_with_pts.first;
                if (audio_frame) {
                    int buffer_size = audio_frame->nb_samples;
                    audio_player->appendBuffer(audio_id,
                                               reinterpret_cast<float *>(audio_frame->data[0]),
                                               buffer_size);
                } else {
                    audio_done = true;
                }
                av_frame_free(&audio_frame);
            }
            if (!video_frame) {
                aout << "end of file" << std::endl;
                is_done = true;
                if(correct)
                    next_page = 4;
                else
                    next_page = 5;
            } else {

                auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0],
                                                                       video_frame->linesize[0],
                                                                       video_frame->height);
                auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1],
                                                                       video_frame->linesize[1],
                                                                       video_frame->height / 2);
                auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2],
                                                                       video_frame->linesize[2],
                                                                       video_frame->height / 2);
                auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3],
                                                                       video_frame->linesize[3],
                                                                       video_frame->height);
                if (video_frame->linesize[3] == 0) {
                    uint8_t temp_a[1] = {255};
                    video_frame->linesize[3] = video_frame->width;
                    a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
                }
                model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                    u_texture_ptr,
                                                                    v_texture_ptr,
                                                                    a_texture_ptr,
                                                                    (float) video_frame->width /
                                                                    video_frame->linesize[0],
                                                                    (float) video_frame->width /
                                                                    video_frame->linesize[1] / 2,
                                                                    (float) video_frame->width /
                                                                    video_frame->linesize[2] / 2,
                                                                    (float) video_frame->width /
                                                                    video_frame->linesize[3],
                                                                    -half_width, +half_width,
                                                                    -half_height,
                                                                    +half_height));
            }
            if(!is_asked and pts>ask_time) {
                is_asked = true;
                still_image=last_model;
                pause();
            }
        }
        else {
            model = still_image;
            model.emplace_back(Model::createModel(text_brand_question, -half_width, +half_width,
                                                  +half_height * 0.6, +half_height));
            model.emplace_back(
                    Model::createModel(text_level1_logos, -half_width * 0.8, +half_width * 0.4,
                                       -half_height * 0.9, half_height * 0.6));
            model.emplace_back(
                    Model::createModel(text_check_button, +half_width * 0.65, half_width * 0.9,
                                       -half_height * 0.5, 0));
            for (int i = 0; i < 4; i++)
                if (checked[i]) {
                    int row = i / 2;
                    int col = i % 2;
                    model.emplace_back(Model::createModel(text_selector,
                                                          -half_width * 0.8 +
                                                          col * half_width * 0.6,
                                                          -half_width * 0.8 +
                                                          (col + 1) * half_width * 0.6,
                                                          -half_height * 0.9 +
                                                          row * half_height * 0.75,
                                                          -half_height * 0.9 +
                                                          (row + 1) * half_height * 0.75));
                }
        }
        if(!is_done)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{
        for (auto i = 0; i < inputBuffer->motionEventsCount; i++) {
            auto &motionEvent = inputBuffer->motionEvents[i];
            auto action = motionEvent.action;
            auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)
                    >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
            auto &pointer = motionEvent.pointers[pointerIndex];
            auto x = GameActivityPointerAxes_getX(&pointer);
            auto y = GameActivityPointerAxes_getY(&pointer);
            if((action & AMOTION_EVENT_ACTION_MASK) == AMOTION_EVENT_ACTION_DOWN){
                checkCheckClick(x,y);
                checkSelectClick(x,y);
            }
        }
    }
private:
    void checkCheckClick(int x, int y){
        if(!is_asked or is_answered)
            return;
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,+half_width*0.65,half_width*0.9) and isBetween(loc.second,-half_height*0.5,0))
        {
            is_answered=true;
            resume();
            for(int i=0;i<4;i++)
                    if(answered[i]!=checked[i])
                        return;
            correct=true;
        }
    }
    void checkSelectClick(int x, int y){
        if(!is_asked or is_answered)
            return;
        auto loc = getRealLocation(x,y);
        if(isBetween(loc.first,-half_width*0.8,+half_width*0.4) and isBetween(loc.second,-half_height*0.9,half_height*0.6))
        {
            memset(checked,0,4);
            int col = (loc.first- (-half_width*0.8))/(half_width*0.6);
            int row = (loc.second- (-half_height*0.9))/(half_height*0.75);
            checked[row*2+col] = 1;
        }
    }
    std::shared_ptr<TextureAsset> text_level1_logos;
    std::shared_ptr<TextureAsset> text_selector;
    std::shared_ptr<TextureAsset> text_brand_question;
    std::shared_ptr<TextureAsset> text_check_button;
    bool checked[4];
    bool answered[4] = {0,0,1,0};
    float ask_time = 9.7;
    bool is_asked = false;
    bool is_answered = false;
    std::vector<Model> still_image;
};
class SubPage005 : public Page{
public:
    SubPage005(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("celeb_fix.mkv");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
        still_image=last_model;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model=still_image;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], video_frame->height);

            if(video_frame->linesize[3]==0){
                uint8_t temp_a[1] = {255};
                video_frame->linesize[3]=video_frame->width;
                a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
            }
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float)video_frame->width/video_frame->linesize[0],
                                                                (float)video_frame->width/video_frame->linesize[1]/2,
                                                                (float)video_frame->width/video_frame->linesize[2]/2,
                                                                (float)video_frame->width/video_frame->linesize[3],
                                                                -half_width, +half_width, -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        if(is_done==false)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
private:
    std::vector<Model> still_image;
};
class SubPage006 : public Page{
public:
    SubPage006(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("wrong_fix.mkv");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
        still_image=last_model;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model=still_image;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], video_frame->height);
            if(video_frame->linesize[3]==0){
                uint8_t temp_a[1] = {255};
                video_frame->linesize[3]=video_frame->width;
                a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
            }
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float)video_frame->width/video_frame->linesize[0],
                                                                (float)video_frame->width/video_frame->linesize[1]/2,
                                                                (float)video_frame->width/video_frame->linesize[2]/2,
                                                                (float)video_frame->width/video_frame->linesize[3],
                                                                -half_width, +half_width, -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        if(is_done==false)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
private:
    std::vector<Model> still_image;
};
class SubPage007 : public Page{
public:
    SubPage007(android_app* pApp):Page(pApp){}
    void initiate() override{
        av_decoder = nullptr;
        audio_player = nullptr;
        is_initiate=is_done=audio_done=false;
        av_decoder = std::make_shared<AVDecoder>();
        audio_player = std::make_shared<AudioPlayer>();
        av_decoder->set_asset_manager(app_->activity->assetManager);
        av_decoder->av_open("page_end_x265.mp4");
        av_decoder->start_thread();
        audio_id=audio_player->addNewFile();
        if(!audio_player->isStarted() and !audio_player->start())
            throw std::runtime_error("Failed to start audio player");
        is_initiate=true;
        is_done=false;
        still_image=last_model;
    }
    std::vector <Model> getModels() override{
        std::vector <Model> model;
        auto video_frame_with_pts = av_decoder->getVideoFrame();
        auto video_frame = video_frame_with_pts.first;
        auto pts = video_frame_with_pts.second;
        while(!av_decoder->is_reach(pts)) {
            if(audio_done)
                continue;
            auto audio_frame_with_pts = av_decoder->getAudioFrame();
            auto audio_frame = audio_frame_with_pts.first;
            if(audio_frame) {
                int buffer_size=audio_frame->nb_samples;
                audio_player->appendBuffer(audio_id,
                                           reinterpret_cast<float *>(audio_frame->data[0]),
                                           buffer_size);
            }
            else{
                audio_done=true;
            }
            av_frame_free(&audio_frame);
        }
        if(!video_frame){
            aout<<"end of file"<<std::endl;
            is_done=true;
            next_page=2;
        }
        else {

            auto y_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[0], video_frame->linesize[0], video_frame->height);
            auto u_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[1], video_frame->linesize[1], video_frame->height / 2);
            auto v_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[2], video_frame->linesize[2], video_frame->height / 2);
            auto a_texture_ptr = TextureAsset::make1DAssetFromData(video_frame->data[3], video_frame->linesize[3], video_frame->height);
            if(video_frame->linesize[3]==0){
                uint8_t temp_a[1] = {255};
                video_frame->linesize[3]=video_frame->width;
                a_texture_ptr = TextureAsset::make1DAssetFromData(temp_a, 1, 1);
            }
            model.emplace_back(Model::createYUVModel_withStride(y_texture_ptr,
                                                                u_texture_ptr,
                                                                v_texture_ptr,
                                                                a_texture_ptr,
                                                                (float)video_frame->width/video_frame->linesize[0],
                                                                (float)video_frame->width/video_frame->linesize[1]/2,
                                                                (float)video_frame->width/video_frame->linesize[2]/2,
                                                                (float)video_frame->width/video_frame->linesize[3],
                                                                -half_width, +half_width, -half_height,
                                                                +half_height));
        }
        av_frame_free(&video_frame);
        if(is_done==false)
            last_model=model;
        return model;
    }
    void handleInput(android_input_buffer* inputBuffer) override{}
private:
    std::vector<Model> still_image;
};

#endif