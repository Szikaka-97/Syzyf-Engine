#pragma once

#include "AudioClip.h"

#include "Debug.h"
#include "GameObject.h"

#include <AL/al.h>

class AudioSource : public GameObject, public ImGuiDrawable {
private:
    ALuint sourceId = 0;
    serialized AudioClip* currentClip = nullptr;

    float volume = 1.0f;
    float pitch = 1.0f;
    bool isLooping = false;
    // For 3D audio to work the clip needs to be mono
    //  otherwise it plays as 2D regardless of whether this is set or not
    bool is2D = false;

    void ApplySettings();

public:
    AudioSource();
    AudioSource(AudioClip* clip);
    virtual ~AudioSource();

    void SetClip(AudioClip* clip);

    void Play();
    void Stop();
    void Pause();

    void SetVolume(float volume);
    void SetPitch(float pitch);
    void SetLooping(bool looping);
    void Set2D(bool isSpatial);
    bool Is2D() const;

    ALuint GetSourceId();

    virtual void DrawImGui() override;
};
