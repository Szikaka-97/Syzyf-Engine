#pragma once

#include <AL/al.h>
#include <vector>

class AudioBuffer
{
public:
    AudioBuffer();
    ~AudioBuffer();

    AudioBuffer(const AudioBuffer&) = delete;
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    void SetData(ALenum format, const std::vector<char>& data, ALsizei sampleRate);

    ALuint GetHandle() const;

private:
    ALuint m_buffer = 0;
};