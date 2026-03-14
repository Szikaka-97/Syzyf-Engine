#pragma once

#include <AL/al.h>

class AudioBuffer;

class AudioSource
{
public:
    AudioSource();
    ~AudioSource();

    AudioSource(const AudioSource&) = delete;
    AudioSource& operator=(const AudioSource&) = delete;

    void SetBuffer(const AudioBuffer& buffer);

    void Play();
    void Pause();
    void Stop();

    void SetLooping(bool looping);
    void SetGain(float gain);
    void SetPitch(float pitch);

    void SetPosition(float x, float y, float z);
    void SetVelocity(float x, float y, float z);

    void SetReferenceDistance(float distance);
    void SetMaxDistance(float distance);
    void SetRolloffFactor(float factor);
    void SetRelative(bool relative);

    bool IsPlaying() const;

private:
    ALuint m_source = 0;
};