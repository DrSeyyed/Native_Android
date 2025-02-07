#include "OboePlayer.h"

OboePlayer::OboePlayer(AAssetManager * assetmanager){
    {
        AAsset *asset = AAssetManager_open(
            assetmanager,
            "output.pcm",
            AASSET_MODE_BUFFER);
        int size = AAsset_getLength(asset);
        const short *buff = static_cast<const short *>(AAsset_getBuffer(asset));
        backmusic.assign(buff,buff+size/2);
        AAsset_close(asset);
    }
    {
        AAsset *asset = AAssetManager_open(
                assetmanager,
                "waterdrop.pcm",
                AASSET_MODE_BUFFER);
        int size = AAsset_getLength(asset);
        const short *buff = static_cast<const short *>(AAsset_getBuffer(asset));
        waterdrop.assign(buff,buff+size/2);
        AAsset_close(asset);
    }
    //mixer.emplace_back(backmusic);
    //mPlace.emplace_back(0);
    //mRepeat.emplace_back(true);
}

int32_t OboePlayer::startAudio() {
    std::lock_guard<std::mutex> lock(mLock);
    oboe::AudioStreamBuilder builder;
    // The builder set methods can be chained for convenience.
    oboe::Result result = builder.setSharingMode(oboe::SharingMode::Exclusive)
            ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
            ->setChannelCount(kChannelCount)
            ->setSampleRate(kSampleRate)
            ->setSampleRateConversionQuality(oboe::SampleRateConversionQuality::Medium)
            ->setFormat(oboe::AudioFormat::I16)
            ->setDataCallback(this)
            ->openStream(mStream);
    if (result != oboe::Result::OK) return (int32_t) result;

    // Typically, start the stream after querying some stream information, as well as some input from the user
    result = mStream->requestStart();
    return (int32_t) result;
}

void OboePlayer::stopAudio() {
    // Stop, close and delete in case not already closed.
    std::lock_guard<std::mutex> lock(mLock);
    if (mStream) {
        mStream->stop();
        mStream->close();
        mStream.reset();
    }
}

oboe::DataCallbackResult OboePlayer::onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
    int16_t *int16Data = (int16_t *) audioData;
    bool end=false;
    for (int i =0; i < numFrames and !end; ++i) {
        int16Data[i]=0;
        for(int j=0;j<mixer.size();j++) {
            int16Data[i] += mixer[j][mPlace[j]++];
            if(mPlace[j] >= mixer[j].size()){
                if(mRepeat[j]){
                    mPlace[j]=0;
                    end=true;
                }
                else{
                    mixer.erase(mixer.begin()+j);
                    mPlace.erase(mPlace.begin()+j);
                    mRepeat.erase(mRepeat.begin()+j);
                    j--;
                }
            }
        }
    }
    return oboe::DataCallbackResult::Continue;
}

void OboePlayer::addDrop() {
    std::lock_guard<std::mutex> lock(mLock);
    mixer.emplace_back(waterdrop);
    mPlace.emplace_back(0);
    mRepeat.emplace_back(false);
}

std::vector<int16_t>* OboePlayer::addAudio(int16_t *frame_buffer, int buffer_size) {
    std::lock_guard<std::mutex> lock(mLock);
    int num=mixer.size();
    mixer.emplace_back(std::vector<int16_t>(frame_buffer,frame_buffer+buffer_size));
    mPlace.emplace_back(0);
    mRepeat.emplace_back(false);
    return &mixer[num];
}
