#pragma once

#include "Resources.h"

#include <AL/al.h>
#include <nlohmann/json_fwd.hpp>

#include <filesystem>

class AudioClip : public Resource {
private:
    ALuint bufferId = 0;
    std::filesystem::path filePath;
    uint64_t hash = 0;

    float duration = 0.0f;

    AudioClip();
public:
    ~AudioClip();

    ALuint GetBufferId();
    float GetDuration() const;

    virtual std::filesystem::path GetPath() const;
    virtual uint64_t GetHash() const;

    static AudioClip* Load(const std::filesystem::path& path);

    nlohmann::json Serialize() const;
    static AudioClip* Deserialize(const nlohmann::json& data);

    AudioClip(const AudioClip&) = delete;
    AudioClip& operator=(const AudioClip&) = delete;
};
