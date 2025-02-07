#ifndef GAME_LOGIC_H
#define GAME_LOGIC_H

#include "AndroidOut.h"
#include "AVDecoder.h"
#include "AudioPlayer.h"
#include "AVDecoderMediaCodec.h"
#include "Model.h"
#include "Page.h"
#include "SubPage.h"
#include <thread>
#include <mutex>
#include <condition_variable>


class GameLogic{
public:
    GameLogic(android_app* pApp):app_(pApp){
        //pages.push_back((Page*)new SubPage0(pApp));
        //pages.push_back((Page*)new SubPage1(pApp));
        //pages.push_back((Page*)new SubPage4(pApp));
        //pages.push_back((Page*)new SubPage7(pApp));
        //pages.push_back((Page*)new SubPage8(pApp));
        //pages.push_back((Page*)new SubPage9(pApp));
        //pages.push_back((Page*)new SubPage10(pApp));
        //pages.push_back((Page*)new SubPage11(pApp));
        //pages.push_back((Page*)new SubPage6(pApp));
        //pages.push_back((Page*)new SubPage5(pApp));
        pages.push_back((Page*)new SubPage001(pApp));
        pages.push_back((Page*)new SubPage002(pApp));
        pages.push_back((Page*)new SubPage003(pApp));
        pages.push_back((Page*)new SubPage004(pApp));
        pages.push_back((Page*)new SubPage005(pApp));
        pages.push_back((Page*)new SubPage006(pApp));
        pages.push_back((Page*)new SubPage007(pApp));
        internal_path = app_->activity->internalDataPath;
        internal_path+=+"/high_score.txt";
        if(std::filesystem::exists(internal_path.c_str())){
            std::ifstream infile;
            infile.open (internal_path);
            infile >> high_score;
            infile.close();
        }
        else{
            std::ofstream outfile (internal_path);
            outfile << 0 << std::endl;
            outfile.close();
            high_score=0;
        }
    }
    ~GameLogic() {
        for(auto& page:pages){
            page->~Page();
        }
    }
    void setScreen(int width,int height,float h_width, float h_height){
        width_=width;
        height_=height;
        half_width = h_width;
        half_height = h_height;
    }
    std::vector<Model> getModels(){
        if(pages[current_page]->isInitiated() and pages[current_page]->isDone()) {
            pages[current_page]->finish();
            current_page=pages[current_page]->getNextPage();
        }
        if(!pages[current_page]->isInitiated() or pages[current_page]->isDone())
            pages[current_page]->initiate();
        pages[current_page]->setScreen(width_,height_,half_width,half_height);
        return pages[current_page]->getModels();
    }
    void handleInput(android_input_buffer* inputBuffer) {
        pages[current_page]->handleInput(inputBuffer);
    }
    void pause(){
        pages[current_page]->pause();
    }
    void resume(){
        pages[current_page]->resume();
    }

private:
    android_app* app_;
    std::vector<Page*> pages;
    std::vector <Model> current_model;
    float half_width,half_height;
    int width_,height_;
    int current_page=0;
    int high_score=0;
    std::string internal_path;
};

#endif
