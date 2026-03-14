#pragma once

#include <GameObject.h>
#include <AudioSource.h>

class AudioBuffer;

class AudioSourceComponent : public GameObject
{
public:
    AudioSourceComponent();

    void SetBuffer(AudioBuffer* buffer);

    void Play();
    void Pause();
    void Stop();

    void SetLooping(bool looping);
    void SetGain(float gain);
    void SetPitch(float pitch);

    void SetReferenceDistance(float distance);
    void SetMaxDistance(float distance);
    void SetRolloffFactor(float factor);

    bool IsPlaying() const;

private:
    AudioSource m_source;
    AudioBuffer* m_buffer = nullptr;
};