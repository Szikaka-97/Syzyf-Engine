#pragma once

#include "audio/AudioClip.h"
#include "audio/AudioSource.h"
#include "audio/AudioSystem.h"

#include <GameObject.h>
#include <Scene.h>
#include <TimeSystem.h>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace GameplayAudio {

inline constexpr const char* BackgroundMusicPath =
    "./res/audio/forgotten-hero-records-calming-medieval-melody-398116.mp3";

inline constexpr const char* BottleBreakSoundPath =
    "./res/audio/freesound_community-house_bottle-break_fast-92421.mp3";

inline constexpr float BackgroundMusicVolume = 0.35f;
inline constexpr float BottleBreakVolume = 0.9f;

class OneShotAudioCleaner : public GameObject {
private:
    float remainingTime = 0.0f;

public:
    explicit OneShotAudioCleaner(float lifetimeSeconds)
        : remainingTime(lifetimeSeconds) {}

    void Update() {
        this->remainingTime -= Time::Delta();

        if (this->remainingTime <= 0.0f) {
            GetScene()->QueueDelete(GetNode());
        }
    }
};

inline void EnsureAudioSystem(Scene& scene) {
    scene.AddComponent<AudioSystem>();
}

inline AudioClip* LoadClip(Scene& scene, const char* path) {
    AudioClip* clip = scene.Resources()->Get<AudioClip>(path);

    if (clip == nullptr) {
        spdlog::error("GameplayAudio: failed to load audio clip: {}", path);
    }

    return clip;
}

inline AudioSource* AddBackgroundMusic(Scene& scene) {
    EnsureAudioSystem(scene);

    AudioClip* musicClip = LoadClip(scene, BackgroundMusicPath);
    if (musicClip == nullptr) {
        return nullptr;
    }

    SceneNode* musicNode = scene.CreateNode("Background Music");
    AudioSource* musicSource = musicNode->AddObject<AudioSource>(musicClip);

    musicSource->Set2D(true);
    musicSource->SetLooping(true);
    musicSource->SetVolume(BackgroundMusicVolume);
    musicSource->Play();

    return musicSource;
}

inline AudioSource* PlayOneShot(
    Scene& scene,
    const char* path,
    const glm::vec3& position,
    float volume,
    bool spatial
) {
    EnsureAudioSystem(scene);

    AudioClip* clip = LoadClip(scene, path);
    if (clip == nullptr) {
        return nullptr;
    }

    SceneNode* audioNode = scene.CreateNode("One Shot Audio");
    audioNode->GlobalTransform().Position() = position;

    AudioSource* source = audioNode->AddObject<AudioSource>(clip);
    source->Set2D(!spatial);
    source->SetLooping(false);
    source->SetVolume(volume);
    source->Play();

    float lifetime = clip->GetDuration();
    if (lifetime <= 0.0f) {
        lifetime = 2.0f;
    }

    audioNode->AddObject<OneShotAudioCleaner>(lifetime + 0.25f);

    return source;
}

inline AudioSource* PlayBottleBreak(Scene& scene, const glm::vec3& position) {
    return PlayOneShot(
        scene,
        BottleBreakSoundPath,
        position,
        BottleBreakVolume,
        true
    );
}

} // namespace GameplayAudio
