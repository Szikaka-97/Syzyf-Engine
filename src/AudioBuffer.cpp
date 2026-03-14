#include <include/AudioBuffer.h>
#include <stdexcept>

AudioBuffer::AudioBuffer()
{
    alGenBuffers(1, &m_buffer);

    if (m_buffer == 0)
        throw std::runtime_error("Failed to create OpenAL buffer");
}

AudioBuffer::~AudioBuffer()
{
    if (m_buffer != 0)
        alDeleteBuffers(1, &m_buffer);
}

void AudioBuffer::SetData(ALenum format, const std::vector<char>& data, ALsizei sampleRate)
{
    alBufferData(m_buffer, format, data.data(), static_cast<ALsizei>(data.size()), sampleRate);

    ALenum error = alGetError();
    if (error != AL_NO_ERROR)
        throw std::runtime_error("Failed to upload audio data to OpenAL buffer");
}

ALuint AudioBuffer::GetHandle() const
{
    return m_buffer;
}