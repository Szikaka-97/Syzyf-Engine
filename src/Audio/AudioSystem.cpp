#include <Audio/AudioSystem.h>

#include <vector>
#include <algorithm>

#include <Audio/AudioBuffer.h>
#include <Audio/AudioSource.h>
#include <Audio/AudioListener.h>
#include <Scene.h>

AudioSystem::AudioSystem()
{
}

AudioSystem::~AudioSystem()
{
    m_playingSounds.clear();
    m_registeredSounds.clear();
}

void AudioSystem::RegisterSound(const std::string& name, AudioBuffer* buffer)
{
    if (buffer == nullptr || name.empty())
        return;

    m_registeredSounds[name] = buffer;
}

bool AudioSystem::HasSound(const std::string& name) const
{
    return m_registeredSounds.find(name) != m_registeredSounds.end();
}

AudioBuffer* AudioSystem::GetSound(const std::string& name) const
{
    auto it = m_registeredSounds.find(name);
    if (it == m_registeredSounds.end())
        return nullptr;

    return it->second;
}

AudioHandler AudioSystem::AddSound(AudioBuffer* buffer, const AudioPlaybackSettings& settings)
{
    if (buffer == nullptr)
        return AudioHandler();

    uint32_t id = m_nextId++;

    PlayingSound sound;
    sound.source = std::make_unique<AudioSource>();
    sound.buffer = buffer;
    sound.settings = settings;

    sound.source->SetBuffer(*buffer);
    sound.source->SetLooping(settings.looping);
    sound.source->SetGain(settings.gain);
    sound.source->SetPitch(settings.pitch);

    sound.source->SetReferenceDistance(1.0f);
    sound.source->SetMaxDistance(std::max(settings.audibleDistance, 0.1f));
    sound.source->SetRolloffFactor(1.0f);

    m_playingSounds.emplace(id, std::move(sound));

    return AudioHandler(id);
}

AudioHandler AudioSystem::Play(AudioBuffer* buffer, const AudioPlaybackSettings& settings)
{
    AudioHandler handle = AddSound(buffer, settings);
    if (!handle.IsValid())
        return handle;

    auto& sound = m_playingSounds[handle.GetId()];
    sound.source->SetRelative(true);
    sound.source->SetPosition(0.0f, 0.0f, 0.0f);
    sound.source->Play();

    return handle;
}

AudioHandler AudioSystem::PlayOnPosition(AudioBuffer* buffer, const glm::vec3& position, const AudioPlaybackSettings& settings)
{
    AudioHandler handle = AddSound(buffer, settings);
    if (!handle.IsValid())
        return handle;

    auto& sound = m_playingSounds[handle.GetId()];
    sound.source->SetRelative(false);
    sound.worldPosition = position;
    sound.source->SetPosition(position.x, position.y, position.z);
    sound.source->Play();

    return handle;
}

AudioHandler AudioSystem::PlayAttached(AudioBuffer* buffer, SceneNode* node, const AudioPlaybackSettings& settings)
{
    AudioHandler handle = AddSound(buffer, settings);
    if (!handle.IsValid())
        return handle;

    auto& sound = m_playingSounds[handle.GetId()];
    sound.source->SetRelative(false);
    sound.attached = true;
    sound.attachedNode = node;

    if (node != nullptr)
    {
        glm::vec3 position = node->GlobalTransform().Position();
        sound.source->SetPosition(position.x, position.y, position.z);
    }

    sound.source->Play();

    return handle;
}

AudioHandler AudioSystem::Play(const std::string& name, const AudioPlaybackSettings& settings)
{
    return Play(GetSound(name), settings);
}

AudioHandler AudioSystem::PlayOnPosition(const std::string& name, const glm::vec3& position, const AudioPlaybackSettings& settings)
{
    return PlayOnPosition(GetSound(name), position, settings);
}

AudioHandler AudioSystem::PlayAttached(const std::string& name, SceneNode* node, const AudioPlaybackSettings& settings)
{
    return PlayAttached(GetSound(name), node, settings);
}

void AudioSystem::Stop(AudioHandler handle)
{
    auto it = m_playingSounds.find(handle.GetId());
    if (it == m_playingSounds.end())
        return;

    it->second.source->Stop();
    m_playingSounds.erase(it);
}

void AudioSystem::Pause(AudioHandler handle)
{
    auto it = m_playingSounds.find(handle.GetId());
    if (it == m_playingSounds.end())
        return;

    it->second.source->Pause();
}

bool AudioSystem::IsPlaying(AudioHandler handle) const
{
    auto it = m_playingSounds.find(handle.GetId());
    if (it == m_playingSounds.end())
        return false;

    return it->second.source->IsPlaying();
}

void AudioSystem::SetGain(AudioHandler handle, float gain)
{
    auto it = m_playingSounds.find(handle.GetId());
    if (it == m_playingSounds.end())
        return;

    it->second.settings.gain = gain;
    it->second.source->SetGain(gain);
}

void AudioSystem::SetPitch(AudioHandler handle, float pitch)
{
    auto it = m_playingSounds.find(handle.GetId());
    if (it == m_playingSounds.end())
        return;

    it->second.settings.pitch = pitch;
    it->second.source->SetPitch(pitch);
}

void AudioSystem::SetAudibleDistance(AudioHandler handle, float distance)
{
    auto it = m_playingSounds.find(handle.GetId());
    if (it == m_playingSounds.end())
        return;

    it->second.settings.audibleDistance = std::max(distance, 0.1f);
    it->second.source->SetMaxDistance(it->second.settings.audibleDistance);
}

void AudioSystem::ApplyDistanceFade(PlayingSound& sound)
{
    if (sound.source == nullptr)
        return;

    if (!sound.attached && sound.worldPosition == glm::vec3(0.0f))
        return;

    glm::vec3 position = sound.worldPosition;
    if (sound.attached && sound.attachedNode != nullptr)
        position = sound.attachedNode->GlobalTransform().Position();

    glm::vec3 listenerPosition = AudioListener::GetPosition();
    float distance = glm::distance(position, listenerPosition);

    float audibleDistance = std::max(sound.settings.audibleDistance, 0.1f);
    float fadeStart = audibleDistance * 0.7f;

    if (distance >= audibleDistance)
    {
        sound.source->SetGain(0.0f);
    }
    else if (distance <= fadeStart)
    {
        sound.source->SetGain(sound.settings.gain);
    }
    else
    {
        float t = (distance - fadeStart) / (audibleDistance - fadeStart);
        t = std::clamp(t, 0.0f, 1.0f);

        float fade = 1.0f - t;
        sound.source->SetGain(sound.settings.gain * fade);
    }
}

void AudioSystem::Update()
{
    std::vector<uint32_t> finished;

    for (auto& [id, sound] : m_playingSounds)
    {
        if (sound.attached)
        {
            if (sound.attachedNode != nullptr)
            {
                glm::vec3 position = sound.attachedNode->GlobalTransform().Position();
                sound.worldPosition = position;
                sound.source->SetPosition(position.x, position.y, position.z);
            }
            else
            {
                finished.push_back(id);
                continue;
            }
        }

        if (!sound.attached)
            ApplyDistanceFade(sound);
        else
            ApplyDistanceFade(sound);

        if (!sound.settings.looping && !sound.source->IsPlaying())
            finished.push_back(id);
    }

    for (uint32_t id : finished)
        m_playingSounds.erase(id);
}