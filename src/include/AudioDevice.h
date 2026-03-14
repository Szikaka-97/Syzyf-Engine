#pragma once

#include <AL/al.h>
#include <AL/alc.h>

class AudioDevice
{
public:
    AudioDevice();
    ~AudioDevice();

    bool IsValid() const;

private:
    ALCdevice* m_device = nullptr;
    ALCcontext* m_context = nullptr;
};