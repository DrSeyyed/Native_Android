#ifndef AV_DECODER_MEDIACODEC_H
#define AV_DECODER_MEDIACODEC_H

#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>
#include <media/NdkMediaCodec.h>
#include <media/NdkMediaExtractor.h>
#include <media/NdkMediaFormat.h>
#include <android/log.h>
#include <unistd.h>   // For ::close()
#include <string>
#include <cstring>    // For std::memcpy

#define LOG_TAG "MediaCodecDecoder"
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)

class MediaCodecDecoder {
public:
    MediaCodecDecoder()
            : extractor(nullptr), codec(nullptr), isCodecConfigured(false), kTimeoutUs(10000){
        extractor = AMediaExtractor_new();
    }

    ~MediaCodecDecoder() {
        close();
    }

    bool openFile(AAssetManager *assetManager, const std::string &filePath) {
        if (!extractor) {
            LOGE("Failed to create media extractor");
            return false;
        }
        AAsset *asset = AAssetManager_open(assetManager, filePath.c_str(), AASSET_MODE_STREAMING);
        if (!asset) {
            LOGE("Failed to open asset %s", filePath.c_str());
            return false;
        }
        off_t assetStart, assetLength;
        int assetFd = AAsset_openFileDescriptor(asset, &assetStart, &assetLength);
        if (assetFd < 0) {
            LOGE("Failed to open file descriptor for asset %s", filePath.c_str());
            AAsset_close(asset);
            return false;
        }
        media_status_t status = AMediaExtractor_setDataSourceFd(extractor, assetFd, assetStart, assetLength);
        ::close(assetFd);  // Correctly close the file descriptor
        AAsset_close(asset);
        if (status != AMEDIA_OK) {
            LOGE("Failed to set data source for extractor");
            return false;
        }

        int numTracks = AMediaExtractor_getTrackCount(extractor);
        for (int i = 0; i < numTracks; ++i) {
            AMediaFormat *format = AMediaExtractor_getTrackFormat(extractor, i);
            const char *mime = nullptr;
            AMediaFormat_getString(format, AMEDIAFORMAT_KEY_MIME, &mime);

            if (mime && strncmp(mime, "video/", 6) == 0) {
                AMediaExtractor_selectTrack(extractor, i);

                codec = AMediaCodec_createDecoderByType(mime);
                if (configureCodec(format)) {
                    isCodecConfigured = true;
                    AMediaFormat_delete(format);
                    return true;
                } else {
                    LOGE("Failed to configure codec");
                    AMediaFormat_delete(format);
                    return false;
                }
            }
            AMediaFormat_delete(format);
        }
        LOGE("No suitable video track found in asset %s", filePath.c_str());
        return false;
    }

    bool decodeFrame(uint8_t* rgbaFrameData, int& width, int& height) {
        if (!isCodecConfigured) {
            LOGE("Codec is not configured");
            return false;
        }

        AMediaCodecBufferInfo bufferInfo;
        ssize_t inputBufIndex = AMediaCodec_dequeueInputBuffer(codec, kTimeoutUs);

        if (inputBufIndex >= 0) {
            size_t bufSize;
            uint8_t* inputBuf = AMediaCodec_getInputBuffer(codec, inputBufIndex, &bufSize);

            ssize_t sampleSize = AMediaExtractor_readSampleData(extractor, inputBuf, bufSize);

            if (sampleSize < 0) {
                LOGI("End of stream reached");
                return false; // End of stream
            }

            uint64_t presentationTimeUs = AMediaExtractor_getSampleTime(extractor);
            AMediaCodec_queueInputBuffer(codec, inputBufIndex, 0, sampleSize, presentationTimeUs, 0);
            AMediaExtractor_advance(extractor);
        } else if (inputBufIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGI("No input buffer available, try again later");
        } else {
            LOGE("Failed to dequeue input buffer: %zd", inputBufIndex);
        }

        ssize_t outputBufIndex = AMediaCodec_dequeueOutputBuffer(codec, &bufferInfo, kTimeoutUs);

        if (outputBufIndex >= 0) {
            AMediaFormat* format = AMediaCodec_getOutputFormat(codec);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
            AMediaFormat_delete(format);
            size_t bufferSize;
            uint8_t* outputBuf = AMediaCodec_getOutputBuffer(codec, outputBufIndex, &bufferSize);

            if (outputBuf) {
                // Assuming YUV420 format, convert to RGBA
                convertToRGBA(outputBuf, width, height, rgbaFrameData);

            } else {
                LOGE("Invalid output buffer or size: %p, size: %zu", outputBuf, bufferSize);
            }

            AMediaCodec_releaseOutputBuffer(codec, outputBufIndex, false);
        } else if (outputBufIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            LOGI("Output format changed");
            AMediaFormat* format = AMediaCodec_getOutputFormat(codec);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
            AMediaFormat_delete(format);
        } else if (outputBufIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGI("No output buffer available, try again later");
        } else {
            LOGE("Failed to dequeue output buffer: %zd", outputBufIndex);
        }

        return true;
    }

    bool decodePureFrame(uint8_t* &rgbaFrameData, int& width, int& height) {
        if (!isCodecConfigured) {
            LOGE("Codec is not configured");
            return false;
        }

        AMediaCodecBufferInfo bufferInfo;
        ssize_t inputBufIndex = AMediaCodec_dequeueInputBuffer(codec, kTimeoutUs);

        if (inputBufIndex >= 0) {
            size_t bufSize;
            uint8_t* inputBuf = AMediaCodec_getInputBuffer(codec, inputBufIndex, &bufSize);

            ssize_t sampleSize = AMediaExtractor_readSampleData(extractor, inputBuf, bufSize);

            if (sampleSize < 0) {
                LOGI("End of stream reached");
                return false; // End of stream
            }

            uint64_t presentationTimeUs = AMediaExtractor_getSampleTime(extractor);
            AMediaCodec_queueInputBuffer(codec, inputBufIndex, 0, sampleSize, presentationTimeUs, 0);
            AMediaExtractor_advance(extractor);
        } else if (inputBufIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGI("No input buffer available, try again later");
        } else {
            LOGE("Failed to dequeue input buffer: %zd", inputBufIndex);
        }

        ssize_t outputBufIndex = AMediaCodec_dequeueOutputBuffer(codec, &bufferInfo, kTimeoutUs);
        if (outputBufIndex >= 0) {

            AMediaFormat* format = AMediaCodec_getOutputFormat(codec);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
            AMediaFormat_delete(format);
            size_t bufferSize;

            uint8_t* outputBuf = AMediaCodec_getOutputBuffer(codec, outputBufIndex, &bufferSize);

            if (outputBuf) {
                // Assuming YUV420 format, convert to RGBA
                //convertToRGBA(outputBuf, width, height, rgbaFrameData);
                rgbaFrameData = new uint8_t[bufferSize];
                memcpy(rgbaFrameData,outputBuf,bufferSize);

            } else {
                LOGE("Invalid output buffer or size: %p, size: %zu", outputBuf, bufferSize);
            }

            AMediaCodec_releaseOutputBuffer(codec, outputBufIndex, false);
        } else if (outputBufIndex == AMEDIACODEC_INFO_OUTPUT_FORMAT_CHANGED) {
            LOGI("Output format changed");
            AMediaFormat* format = AMediaCodec_getOutputFormat(codec);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_WIDTH, &width);
            AMediaFormat_getInt32(format, AMEDIAFORMAT_KEY_HEIGHT, &height);
            AMediaFormat_delete(format);

        } else if (outputBufIndex == AMEDIACODEC_INFO_TRY_AGAIN_LATER) {
            LOGI("No output buffer available, try again later");
        } else {
            LOGE("Failed to dequeue output buffer: %zd", outputBufIndex);
        }

        return true;
    }
    void close() {
        if (codec) {
            AMediaCodec_stop(codec);
            AMediaCodec_delete(codec);
            codec = nullptr;
        }

        if (extractor) {
            AMediaExtractor_delete(extractor);
            extractor = nullptr;
        }

        isCodecConfigured = false;
    }

private:
    AMediaExtractor* extractor;
    AMediaCodec* codec;
    bool isCodecConfigured;
    const int64_t kTimeoutUs;

    bool configureCodec(AMediaFormat* format) {
        media_status_t status = AMediaCodec_configure(codec, format, nullptr, nullptr, 0);
        if (status != AMEDIA_OK) {
            LOGE("Failed to configure codec: %d", status);
            return false;
        }

        status = AMediaCodec_start(codec);
        if (status != AMEDIA_OK) {
            LOGE("Failed to start codec: %d", status);
            return false;
        }

        return true;
    }

    void convertToRGBA(const uint8_t* srcBuffer, int width, int height, uint8_t* dstBuffer) {
        int yPlaneSize = width * height;
        int uvPlaneSize = (width / 2) * (height / 2);

        const uint8_t* yPlane = srcBuffer;
        const uint8_t* uPlane = srcBuffer + yPlaneSize;
        const uint8_t* vPlane = uPlane + uvPlaneSize;

        int dstStride = width * 4;  // RGBA has 4 bytes per pixel
        for (int y = 0; y < height; ++y) {
            const uint8_t* yRow = yPlane + y * width;
            const uint8_t* uRow = uPlane + (y / 2) * (width / 2);
            const uint8_t* vRow = vPlane + (y / 2) * (width / 2);

            for (int x = 0; x < width; ++x) {
                uint8_t Y = yRow[x];
                uint8_t U = uRow[x / 2];
                uint8_t V = vRow[x / 2];

                int C = Y - 16;
                int D = U - 128;
                int E = V - 128;

                int R = clip((298 * C + 409 * E + 128) >> 8);
                int G = clip((298 * C - 100 * D - 208 * E + 128) >> 8);
                int B = clip((298 * C + 516 * D + 128) >> 8);

                int dstIndex = y * dstStride + x * 4;
                dstBuffer[dstIndex]     = R;
                dstBuffer[dstIndex + 1] = G;
                dstBuffer[dstIndex + 2] = B;
                dstBuffer[dstIndex + 3] = 255; // Alpha
            }
        }
    }

    uint8_t clip(int value) {
        return (value < 0) ? 0 : ((value > 255) ? 255 : value);
    }
};

#endif
