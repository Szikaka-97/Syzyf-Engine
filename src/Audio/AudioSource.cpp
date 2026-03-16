#include <Audio/AudioSource.h>
#include <Audio/AudioBuffer.h>

#include <stdexcept>

AudioSource::AudioSource()
{
    alGenSources(1, &m_source);

    if (m_source == 0)
        throw std::runtime_error("Failed to create OpenAL source");
}

AudioSource::~AudioSource()
{
    if (m_source != 0)
        alDeleteSources(1, &m_source);
}

void AudioSource::SetBuffer(const AudioBuffer& buffer)
{
    alSourcei(m_source, AL_BUFFER, static_cast<ALint>(buffer.GetHandle()));
}

void AudioSource::Play()
{
    alSourcePlay(m_source);
}

void AudioSource::Pause()
{
    alSourcePause(m_source);
}

void AudioSource::Stop()
{
    alSourceStop(m_source);
}

void AudioSource::SetLooping(bool looping)
{
    alSourcei(m_source, AL_LOOPING, looping ? AL_TRUE : AL_FALSE);
}

void AudioSource::SetGain(float gain)
{
    alSourcef(m_source, AL_GAIN, gain);
}

void AudioSource::SetPitch(float pitch)
{
    alSourcef(m_source, AL_PITCH, pitch);
}

void AudioSource::SetPosition(float x, float y, float z)
{
    alSource3f(m_source, AL_POSITION, x, y, z);
}

void AudioSource::SetVelocity(float x, float y, float z)
{
    alSource3f(m_source, AL_VELOCITY, x, y, z);
}

void AudioSource::SetReferenceDistance(float distance)
{
    alSourcef(m_source, AL_REFERENCE_DISTANCE, distance);
}

void AudioSource::SetMaxDistance(float distance)
{
    alSourcef(m_source, AL_MAX_DISTANCE, distance);
}

void AudioSource::SetRolloffFactor(float factor)
{
    alSourcef(m_source, AL_ROLLOFF_FACTOR, factor);
}

void AudioSource::SetRelative(bool relative)
{
    alSourcei(m_source, AL_SOURCE_RELATIVE, relative ? AL_TRUE : AL_FALSE);
}

bool AudioSource::IsPlaying() const
{
    ALint state = 0;
    alGetSourcei(m_source, AL_SOURCE_STATE, &state);
    return state == AL_PLAYING;
}