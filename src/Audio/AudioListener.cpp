#include <Audio/AudioListener.h>
#include <AL/al.h>

glm::vec3 AudioListener::s_position = glm::vec3(0.0f);

void AudioListener::SetPosition(float x, float y, float z)
{
    s_position = glm::vec3(x, y, z);
    alListener3f(AL_POSITION, x, y, z);
}

void AudioListener::SetPosition(const glm::vec3& position)
{
    SetPosition(position.x, position.y, position.z);
}

void AudioListener::SetVelocity(float x, float y, float z)
{
    alListener3f(AL_VELOCITY, x, y, z);
}

void AudioListener::SetVelocity(const glm::vec3& velocity)
{
    SetVelocity(velocity.x, velocity.y, velocity.z);
}

void AudioListener::SetOrientation(const glm::vec3& forward, const glm::vec3& up)
{
    float orientation[6] =
    {
        forward.x, forward.y, forward.z,
        up.x, up.y, up.z
    };

    alListenerfv(AL_ORIENTATION, orientation);
}

void AudioListener::SetGain(float gain)
{
    alListenerf(AL_GAIN, gain);
}

glm::vec3 AudioListener::GetPosition()
{
    return s_position;
}