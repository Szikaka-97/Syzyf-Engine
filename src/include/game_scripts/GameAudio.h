#pragma once

#include "audio/AudioClip.h"
#include "audio/AudioSource.h"
#include "audio/AudioSystem.h"

#include <Scene.h>
#include <TimeSystem.h>

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

#include <glm/glm.hpp>
#include <spdlog/spdlog.h>

namespace GameAudio {

inline constexpr const char* MainMenuMusicPath =
    "./res/audio/fantasyBackground/openmindaudio-fantasy-cinematic-background-mythic-world-469173.mp3";
inline constexpr const char* BaseAmbientPath =
    "./res/audio/kuchnia/freesound_community-017562_sink-draining-amp-kitchen-ambiencewav-55137.mp3";
inline constexpr const char* CraftingAmbientPath =
    "./res/audio/stacja/soundreality-fire-crackling-528620.mp3";
inline constexpr const char* TutorialAmbientPath =
    "./res/audio/dungeon/freesound_community-a_dungeon_ambience_loop-79423.mp3";
inline constexpr const char* DungeonAmbientPath =
    "./res/audio/dungeon/freesound_community-a_dungeon_ambience_loop-79423.mp3";

inline constexpr const char* BottleBreakPath01 =
    "./res/audio/szklo/universfield-glass-bottle-smash-277554.mp3";
inline constexpr const char* BottleBreakPath02 =
    "./res/audio/szklo/eaglaxle-glass-break-3-530786.mp3";
inline constexpr const char* BottleBreakPath03 =
    "./res/audio/szklo/stu9-glass-crack-363162.mp3";

inline constexpr const char* CraftingBlowerPath =
    "./res/audio/stacja/freesound_community-air-blowing-1-96033.mp3";
inline constexpr const char* CraftingBoilingLoopPath =
    "./res/audio/bulgotanie/freesound_community-boiling-water-sound-62556.mp3";
inline constexpr const char* CraftingLidPath =
    "./res/audio/Click.wav";
inline constexpr const char* CraftingValvePath =
    "./res/audio/stacja/freesound_community-sfx-cassette-tape-motor-30698.mp3";
inline constexpr const char* CraftingStageChangePath =
    "./res/audio/Click.wav";

inline constexpr const char* UiClickPath =
    "./res/audio/Click.wav";
inline constexpr const char* PlayerBreathingPath =
    "./res/audio/freesound_community-male-breathing-84775.mp3";
inline constexpr const char* WalkFootstepPath01 =
    "./res/audio/chodzenie/freesound_community-heavy-footsteps-walking-35722.mp3";
inline constexpr const char* WalkFootstepPath02 =
    "./res/audio/chodzenie/freesound_community-walking-46245.mp3";
inline constexpr const char* RunFootstepPath01 =
    "./res/audio/bieganie/freesound_community-running-1-6846.mp3";
inline constexpr const char* RunFootstepPath02 =
    "./res/audio/bieganie/freesound_community-running-6358.mp3";
inline constexpr const char* RunFootstepPath03 =
    "./res/audio/bieganie/freesounds123-running-363346.mp3";
inline constexpr const char* RunFootstepPath04 =
    "./res/audio/bieganie/freesoundsxx-running-on-concrete-268478.mp3";
inline constexpr const char* WindPath01 =
    "./res/audio/wiatr/dragon-studio-bitter-cold-wind-482876.mp3";
inline constexpr const char* WindPath02 =
    "./res/audio/wiatr/dragon-studio-eerie-wind-478386.mp3";
inline constexpr const char* MouseSqueakPath01 =
    "./res/audio/mysz/freesound_community-double-squeak-2-103875.mp3";
inline constexpr const char* MouseSqueakPath02 =
    "./res/audio/mysz/tanweraman-mouse-squeak-261126.mp3";
inline constexpr const char* AlcoholicHiccupPath =
    "./res/audio/alkoholik/freesound_community-male-hiccup-67892.mp3";
inline constexpr const char* BoneMovementPath =
    "./res/audio/ko#U015bci/freesound_community-bones-and-flesh-movement-98677.mp3";
inline constexpr const char* BoneRattlePath =
    "./res/audio/ko#U015bci/freesound_community-rattling-bones-105394.mp3";

class OneShotAudioLifetime : public GameObject {
  private:
    float secondsLeft = 1.0f;

  public:
    explicit OneShotAudioLifetime(float lifetimeSeconds) :
        secondsLeft(lifetimeSeconds) {}

    void Update() {
        secondsLeft -= Time::UnscaledDelta();

        if (secondsLeft <= 0.0f) {
            GetScene()->QueueDelete(GetNode());
        }
    }
};

inline void EnsureAudioSystem(Scene& scene) {
    scene.GetOrCreateComponent<AudioSystem>();
}

inline bool FileExists(const char* path) {
    return path != nullptr && std::filesystem::exists(std::filesystem::path(path));
}

inline AudioClip* LoadClip(Scene& scene, const char* path) {
    if (!FileExists(path)) {
        spdlog::warn("GameAudio: missing audio file: {}", path ? path : "<null>");
        return nullptr;
    }

    return scene.Resources()->Get<AudioClip>(path);
}

inline bool CanPlayNow(const std::string& key, float cooldownSeconds) {
    if (cooldownSeconds <= 0.0f) {
        return true;
    }

    static std::unordered_map<std::string, float> lastPlayedTimes;

    const float now = Time::Current();
    auto found = lastPlayedTimes.find(key);

    if (found != lastPlayedTimes.end() && now - found->second < cooldownSeconds) {
        return false;
    }

    lastPlayedTimes[key] = now;
    return true;
}

inline std::vector<ALuint>& ActiveLooping2DSources() {
    static std::vector<ALuint> sources;
    return sources;
}

inline void StopActiveLooping2D() {
    auto& sources = ActiveLooping2DSources();

    for (ALuint sourceId : sources) {
        if (!alIsSource(sourceId)) {
            continue;
        }

        alSourceStop(sourceId);
        alSourcei(sourceId, AL_LOOPING, AL_FALSE);
    }

    sources.clear();
}

inline void RegisterLooping2D(AudioSource* source) {
    if (source == nullptr) {
        return;
    }

    ActiveLooping2DSources().push_back(source->GetSourceId());
}

inline AudioSource* AddLooping2D(
    Scene& scene,
    const std::string& nodeName,
    const char* clipPath,
    float volume
) {
    EnsureAudioSystem(scene);
    StopActiveLooping2D();

    AudioClip* clip = LoadClip(scene, clipPath);

    if (clip == nullptr) {
        return nullptr;
    }

    SceneNode* audioNode = scene.CreateNode(nodeName);
    auto* source = audioNode->AddObject<AudioSource>(clip);
    source->Set2D(true);
    source->SetLooping(true);
    source->SetVolume(volume);
    source->Play();
    RegisterLooping2D(source);

    return source;
}

inline AudioSource* PlayOneShot(
    Scene& scene,
    const std::string& nodeName,
    const char* clipPath,
    const glm::vec3& worldPosition,
    float volume,
    bool is2D,
    float cooldownSeconds = 0.0f,
    float pitch = 1.0f
) {
    if (!CanPlayNow(nodeName + clipPath, cooldownSeconds)) {
        return nullptr;
    }

    EnsureAudioSystem(scene);

    AudioClip* clip = LoadClip(scene, clipPath);

    if (clip == nullptr) {
        return nullptr;
    }

    SceneNode* audioNode = scene.CreateNode(nodeName);
    audioNode->GlobalTransform().Position() = worldPosition;

    auto* source = audioNode->AddObject<AudioSource>(clip);
    source->Set2D(is2D);
    source->SetLooping(false);
    source->SetVolume(volume);
    source->SetPitch(pitch);
    source->Play();

    audioNode->AddObject<OneShotAudioLifetime>(clip->GetDuration() + 0.35f);

    return source;
}

inline AudioSource* PlayOneShot2D(
    Scene& scene,
    const std::string& nodeName,
    const char* clipPath,
    float volume,
    float cooldownSeconds = 0.0f,
    float pitch = 1.0f
) {
    return PlayOneShot(
        scene,
        nodeName,
        clipPath,
        glm::vec3(0.0f),
        volume,
        true,
        cooldownSeconds,
        pitch
    );
}

inline AudioSource* PlayOneShot3D(
    Scene& scene,
    const std::string& nodeName,
    const char* clipPath,
    const glm::vec3& worldPosition,
    float volume,
    float cooldownSeconds = 0.0f,
    float pitch = 1.0f
) {
    return PlayOneShot(
        scene,
        nodeName,
        clipPath,
        worldPosition,
        volume,
        false,
        cooldownSeconds,
        pitch
    );
}

inline const char* NextBottleBreakPath() {
    static std::size_t nextIndex = 0;
    static const std::vector<const char*> paths = {
        BottleBreakPath01,
        BottleBreakPath02,
        BottleBreakPath03
    };

    const char* path = paths[nextIndex % paths.size()];
    ++nextIndex;

    return path;
}

inline void StopAndForget(AudioSource*& source) {
    if (source != nullptr) {
        source->Stop();
        source = nullptr;
    }
}

} // namespace GameAudio
