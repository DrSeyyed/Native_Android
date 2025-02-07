#ifndef SECONDGAMEPROJECT_OBOEPLAYER_H
#define SECONDGAMEPROJECT_OBOEPLAYER_H

#include <oboe/Oboe.h>
#include <math.h>
#include <vector>

#include <android/asset_manager.h>

class OboePlayer: public oboe::AudioStreamDataCallback {
public:

    OboePlayer(AAssetManager * assetmanager);
    virtual ~OboePlayer() = default;

    // Call this from Activity onResume()
    int32_t startAudio();

    // Call this from Activity onPause()
    void stopAudio();

    //Add drop sound
    void addDrop();
    std::vector<int16_t>* addAudio(int16_t* frame_buffer, int buffer_size );

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) override;

private:
    std::vector<std::vector<int16_t>> mixer;
    std::vector<int16_t> backmusic,waterdrop;
    std::mutex         mLock;
    std::shared_ptr<oboe::AudioStream> mStream;

    // Stream params
    static int constexpr kChannelCount = 1;
    static int constexpr kSampleRate = 16000;
    // Keeps track of where the wave is
    std::vector<int> mPlace;
    std::vector<bool> mRepeat;
};

#endif //SECONDGAMEPROJECT_OBOEPLAYER_H
