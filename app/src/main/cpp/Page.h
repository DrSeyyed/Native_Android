#ifndef PAGE_H
#define PAGE_H

extern "C" {
#include <game-activity/native_app_glue/android_native_app_glue.h>
}

class Page{
public:
    Page(android_app* pApp): app_(pApp){}
    ~Page(){
        if(av_decoder)
            av_decoder->~AVDecoder();
        if(audio_player)
            audio_player->~AudioPlayer();
    }
    void setScreen(int width,int height,float h_width, float h_height){
        width_=width;
        height_=height;
        half_width = h_width;
        half_height = h_height;
    }
    bool isInitiated(){
        return is_initiate;
    }
    bool isDone(){
        return is_done;
    }
    void finish(){
        if(audio_player)
            audio_player->stop();
    }
    void pause(){
        if(audio_player)
            audio_player->stop();
        if(av_decoder)
            av_decoder->pause();
    }
    void resume(){
        if(audio_player)
            audio_player->start();
        if(av_decoder)
            av_decoder->resume();
    }
    int getNextPage(){
        return next_page;
    }
    virtual void initiate()=0;
    virtual std::vector <Model> getModels()=0;
    virtual void handleInput(android_input_buffer* inputBuffer)=0;
protected:
    android_app* app_;
    std::shared_ptr<AVDecoder> av_decoder;
    std::shared_ptr<AudioPlayer> audio_player;
    float half_width,half_height;
    int width_,height_;
    int audio_id;
    bool is_initiate=false,is_done=false, audio_done=false;
    int next_page;
    std::pair<float,float> getRealLocation(int x, int y){
        float center_x=width_/2;
        float center_y=height_/2;
        float step=height_/(half_height*2);
        float real_x= (x-center_x)/step;
        float real_y= (center_y-y)/step;
        return std::make_pair(real_x,real_y);
    }
    bool isBetween(float x, float lower,float higher){
        if(x<=lower or x>=higher)
            return false;
        return true;
    }
};

#endif
