#ifndef AUDIO_PLAYER_H
#define AUDIO_PLAYER_H

#include <oboe/Oboe.h>
#include <mutex>
#include <vector>
#include <queue>
#include <unordered_map>
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <atomic>
#include <chrono>

class AudioPlayer : public oboe::AudioStreamCallback {
private:
    struct AudioBuffer {
        std::queue<float> data;  // Use float for FLTP format

        void append(float* buffer, size_t numSamples) {
            for (size_t i = 0; i < numSamples; ++i) {
                data.push(buffer[i]);
            }
        }

        size_t availableReadSamples() const {
            return data.size();
        }

        void read(float* output, size_t numSamples) {
            for (size_t i = 0; i < numSamples; ++i) {
                output[i] = data.front();
                data.pop();
            }
        }
    };

    oboe::AudioStream *stream;
    std::mutex bufferMutex;
    std::unordered_map<int, AudioBuffer> audioBuffers;
    std::vector<float> mixBuffer;  // Use float for mixing in FLTP format
    int32_t mixBufferSize;
    std::atomic<int> nextFileId;
    std::chrono::system_clock::time_point start_time;
    bool started = false;

    float clampToFloat(float sample) {
        // Clamp the float sample to the range [-1.0, 1.0]
        if (sample > 1.0f) return 1.0f;
        if (sample < -1.0f) return -1.0f;
        return sample;
    }

    oboe::DataCallbackResult onAudioReady(oboe::AudioStream *oboeStream, void *audioData, int32_t numFrames) {
        std::lock_guard<std::mutex> lock(bufferMutex);

        // Clear the mix buffer
        if (mixBufferSize < numFrames) {
            mixBuffer.resize(numFrames);
            mixBufferSize = numFrames;
        }
        std::fill(mixBuffer.begin(), mixBuffer.end(), 0.0f);

        // Mix all audio buffers into the mix buffer
        size_t framesToCopy;
        for (auto& bufferPair : audioBuffers) {
            AudioBuffer& audioBuffer = bufferPair.second;
            size_t framesAvailable = audioBuffer.availableReadSamples();
            framesToCopy = std::min(static_cast<size_t>(numFrames), framesAvailable);

            if (framesToCopy > 0) {
                std::vector<float> tempBuffer(framesToCopy);
                audioBuffer.read(tempBuffer.data(), framesToCopy);
                for (size_t i = 0; i < framesToCopy; ++i) {
                    mixBuffer[i] += tempBuffer[i];
                    mixBuffer[i] = clampToFloat(mixBuffer[i]);
                }
            }
        }

        // Fill the remaining frames with silence if needed
        std::fill(mixBuffer.begin() + framesToCopy, mixBuffer.end(), 0.0f);

        // Copy the mix buffer to the audio data (assuming float output in Oboe)
        std::memcpy(audioData, mixBuffer.data(), numFrames * sizeof(float));

        return oboe::DataCallbackResult::Continue;
    }

public:
    AudioPlayer() : stream(nullptr), mixBufferSize(0), nextFileId(1) {}
    ~AudioPlayer() {
        stop();
    }

    bool start() {
        oboe::AudioStreamBuilder builder;
        builder.setDirection(oboe::Direction::Output)
                ->setPerformanceMode(oboe::PerformanceMode::LowLatency)
                ->setSharingMode(oboe::SharingMode::Exclusive)
                ->setFormat(oboe::AudioFormat::Float)  // Set format to float
                ->setChannelCount(oboe::ChannelCount::Mono)
                ->setSampleRate(44100)
                ->setCallback(this);

        oboe::Result result = builder.openStream(&stream);
        if (result != oboe::Result::OK) {
            return false;
        }

        stream->start();
        start_time = std::chrono::system_clock::now();
        started = true;
        return true;
    }

    void stop() {
        if (stream) {
            stream->stop();
            stream->close();
            stream = nullptr;
            started = false;
        }
    }

    bool isStarted(){
        std::lock_guard<std::mutex> lock(bufferMutex);
        return started;
    }

    int addNewFile() {
        std::lock_guard<std::mutex> lock(bufferMutex);
        int fileId = nextFileId++;
        audioBuffers[fileId] = AudioBuffer();
        return fileId;
    }

    void deleteFile(int fileId) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        audioBuffers.erase(fileId);
    }

    void appendBuffer(int fileId, float* buffer, int32_t numSamples) {
        std::lock_guard<std::mutex> lock(bufferMutex);
        auto it = audioBuffers.find(fileId);
        if (it != audioBuffers.end()) {
            it->second.append(buffer, numSamples);
        }
    }

    double get_duration(){
        auto now = std::chrono::system_clock::now();
        std::chrono::duration<double> diff = now - start_time;
        return diff.count();
    }
};

#endif
