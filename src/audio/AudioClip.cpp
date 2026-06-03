#include "audio/AudioClip.h"
#include "Resources.h"

#include <dr_flac.h>
#include <dr_mp3.h>
#include <dr_wav.h>
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

AudioClip::AudioClip() {
    alGenBuffers(1, &bufferId);
}

AudioClip::~AudioClip() {
    alDeleteBuffers(1, &bufferId);
}

ALuint AudioClip::GetBufferId() {
    return this->bufferId;
}

std::filesystem::path AudioClip::GetPath() const {
    return this->filePath;
}

uint64_t AudioClip::GetHash() const {
    return this->hash;
}

AudioClip* AudioClip::Load(const std::filesystem::path& path) {
    std::string extension = path.extension().string();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::tolower);

    unsigned int channels = 0;
    unsigned int sampleRate = 0;
    drwav_uint64 totalPCMFrameCount = 0;
    int16_t* pSampleData = nullptr;

    std::string pathString = path.string();

    if (extension == ".wav") {
        pSampleData = drwav_open_file_and_read_pcm_frames_s16(pathString.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
    } else if (extension == ".mp3") {
        drmp3_config config;
        pSampleData = drmp3_open_file_and_read_pcm_frames_s16(
            pathString.c_str(), &config, &totalPCMFrameCount, nullptr);
        if (pSampleData) {
            channels = config.channels;
            sampleRate = config.sampleRate;
        }
    } else if (extension == ".flac") {
        pSampleData = drflac_open_file_and_read_pcm_frames_s16(
            pathString.c_str(), &channels, &sampleRate, &totalPCMFrameCount, nullptr);
    } else {
        spdlog::error("AudioClip::Load: Unsupported audio format: {}", extension);
        return nullptr;
    }

    if (!pSampleData) {
        spdlog::error("AudioClip::Load: Failed to load audio file: {}", pathString);
        return nullptr;
    }

    ALenum format = (channels == 1) ? AL_FORMAT_MONO16 : AL_FORMAT_STEREO16;
    ALsizei dataSize = (ALsizei)(totalPCMFrameCount * channels * sizeof(int16_t));

    AudioClip* clip = new AudioClip();
    clip->filePath = path;
    clip->hash = std::hash<std::string>{}(pathString);

    alBufferData(clip->bufferId, format, pSampleData, dataSize, sampleRate);

    if (extension == ".wav") drwav_free(pSampleData, nullptr);
    else if (extension == ".mp3") drmp3_free(pSampleData, nullptr);
    else if (extension == ".flac") drflac_free(pSampleData, nullptr);

    spdlog::info("AudioClip::Load: Loaded: {}", pathString);
    return clip;
}

nlohmann::json AudioClip::Serialize() const {
    nlohmann::json data;
    data["path"] = this->GetPath();
    return data;
}

AudioClip* AudioClip::Deserialize(const nlohmann::json& data) {
    std::filesystem::path clipPath = data["path"];

    if (!std::filesystem::exists(clipPath) || !std::filesystem::is_regular_file(clipPath)) {
        spdlog::error("AudioClip::Deserialize: File does not exist: {}", clipPath.string());
        return nullptr;
    }

    return ResourceDatabase::Global->Get<AudioClip>(clipPath);
}
