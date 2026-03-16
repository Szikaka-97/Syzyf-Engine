#include <Audio/AudioDevice.h>

#include <stdexcept>

AudioDevice::AudioDevice()
{
    m_device = alcOpenDevice(nullptr);
    if (!m_device)
        throw std::runtime_error("Failed to open OpenAL device");

    m_context = alcCreateContext(m_device, nullptr);
    if (!m_context)
    {
        alcCloseDevice(m_device);
        m_device = nullptr;
        throw std::runtime_error("Failed to create OpenAL context");
    }

    if (!alcMakeContextCurrent(m_context))
    {
        alcDestroyContext(m_context);
        alcCloseDevice(m_device);
        m_context = nullptr;
        m_device = nullptr;
        throw std::runtime_error("Failed to make OpenAL context current");
    }
}

AudioDevice::~AudioDevice()
{
    alcMakeContextCurrent(nullptr);

    if (m_context)
        alcDestroyContext(m_context);

    if (m_device)
        alcCloseDevice(m_device);
}

bool AudioDevice::IsValid() const
{
    return m_device != nullptr && m_context != nullptr;
}