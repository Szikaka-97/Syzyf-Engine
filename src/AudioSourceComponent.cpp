#include <AudioSourceComponent.h>

AudioSourceComponent::AudioSourceComponent()
{
    m_source.SetRelative(false);
}

void AudioSourceComponent::SetBuffer(AudioBuffer* buffer)
{
    m_buffer = buffer;

    if (m_buffer != nullptr)
        m_source.SetBuffer(*m_buffer);
}

void AudioSourceComponent::Play()
{
    if (m_buffer != nullptr)
        m_source.Play();
}

void AudioSourceComponent::Pause()
{
    m_source.Pause();
}

void AudioSourceComponent::Stop()
{
    m_source.Stop();
}

void AudioSourceComponent::SetLooping(bool looping)
{
    m_source.SetLooping(looping);
}

void AudioSourceComponent::SetGain(float gain)
{
    m_source.SetGain(gain);
}

void AudioSourceComponent::SetPitch(float pitch)
{
    m_source.SetPitch(pitch);
}

void AudioSourceComponent::SetReferenceDistance(float distance)
{
    m_source.SetReferenceDistance(distance);
}

void AudioSourceComponent::SetMaxDistance(float distance)
{
    m_source.SetMaxDistance(distance);
}

void AudioSourceComponent::SetRolloffFactor(float factor)
{
    m_source.SetRolloffFactor(factor);
}

bool AudioSourceComponent::IsPlaying() const
{
    return m_source.IsPlaying();
}