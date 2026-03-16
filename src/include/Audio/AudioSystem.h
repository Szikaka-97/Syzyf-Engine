#pragma once

#include <memory>
#include <string>
#include <unordered_map>

#include <glm/glm.hpp>

#include <Audio/AudioHandler.h>

class AudioBuffer;
class AudioSource;
class SceneNode;

struct AudioPlaybackSettings
{
    bool looping = false;
    float gain = 1.0f;
    float pitch = 1.0f;
    float audibleDistance = 10.0f;
};

class AudioSystem
{
public:
    AudioSystem();
    ~AudioSystem();

    void RegisterSound(const std::string& name, AudioBuffer* buffer);
    bool HasSound(const std::string& name) const;
    AudioBuffer* GetSound(const std::string& name) const;

    AudioHandler Play(AudioBuffer* buffer, const AudioPlaybackSettings& settings = {});
    AudioHandler PlayOnPosition(AudioBuffer* buffer, const glm::vec3& position, const AudioPlaybackSettings& settings = {});
    AudioHandler PlayAttached(AudioBuffer* buffer, SceneNode* node, const AudioPlaybackSettings& settings = {});
    AudioHandler Play(const std::string& name, const AudioPlaybackSettings& settings = {});
    AudioHandler PlayOnPosition(const std::string& name, const glm::vec3& position, const AudioPlaybackSettings& settings = {});
    AudioHandler PlayAttached(const std::string& name, SceneNode* node, const AudioPlaybackSettings& settings = {});

    void Stop(AudioHandler handle);
    void Pause(AudioHandler handle);

    bool IsPlaying(AudioHandler handle) const;

    void SetGain(AudioHandler handle, float gain);
    void SetPitch(AudioHandler handle, float pitch);
    void SetAudibleDistance(AudioHandler handle, float distance);

    void Update();

private:
    struct PlayingSound
    {
        std::unique_ptr<AudioSource> source;
        AudioBuffer* buffer = nullptr;

        SceneNode* attachedNode = nullptr;
        glm::vec3 worldPosition = glm::vec3(0.0f);

        AudioPlaybackSettings settings;

        bool attached = false;
    };

    uint32_t m_nextId = 1;
    std::unordered_map<uint32_t, PlayingSound> m_playingSounds;
    std::unordered_map<std::string, AudioBuffer*> m_registeredSounds;

    AudioHandler AddSound(AudioBuffer* buffer, const AudioPlaybackSettings& settings);
    void ApplyDistanceFade(PlayingSound& sound);
};