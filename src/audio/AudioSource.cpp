#include "audio/AudioSource.h"
#include "al.h"

#include <imgui.h>

AudioSource::AudioSource() {
    alGenSources(1, &sourceId);
    ApplySettings();
}

AudioSource::AudioSource(AudioClip* clip) {
    alGenSources(1, &sourceId);
    this->currentClip = clip;
    ApplySettings();
}

AudioSource::~AudioSource() {
    Stop();
    alSourcei(sourceId, AL_BUFFER, 0);
    alDeleteSources(1, &sourceId);
}

void AudioSource::ApplySettings() {
    alSourcef(sourceId, AL_GAIN, volume);
    alSourcef(sourceId, AL_PITCH, pitch);
    alSourcei(sourceId, AL_LOOPING, isLooping ? AL_TRUE : AL_FALSE);

    if (this->is2D) {
        alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSource3f(sourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    } else {
        alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
        alSource3f(sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
    }

    alGetError();

    ALint state;
    alGetSourcei(sourceId, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING && state != AL_PAUSED) {
        if (this->currentClip) {
            alSourcei(sourceId, AL_BUFFER, currentClip->GetBufferId());
        } else {
            alSourcei(sourceId, AL_BUFFER, 0);
        }
    }

    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        spdlog::error("AudioSource::ApplySettings: OpenAL error 0x{:X}", err);
    }
}

void AudioSource::Set2D(bool isSpatial) {
    this->is2D = isSpatial;

    if (this->is2D) {
        alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_TRUE);
        alSource3f(sourceId, AL_POSITION, 0.0f, 0.0f, 0.0f);
        alSource3f(sourceId, AL_VELOCITY, 0.0f, 0.0f, 0.0f);
    } else {
        alSourcei(sourceId, AL_SOURCE_RELATIVE, AL_FALSE);
    }
}

bool AudioSource::Is2D() const {
    return this->is2D;
}

void AudioSource::SetClip(AudioClip* clip) {
    this->currentClip = clip;

    if (this->currentClip == nullptr) {
        spdlog::warn("AudioSource::SetClip: The clip is null");
        alSourcei(sourceId, AL_BUFFER, 0);
        return;
    }

    alGetError();
    alSourcei(sourceId, AL_BUFFER, currentClip->GetBufferId());

    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        spdlog::error("AudioSource::SetClip: OpenAL failed to bind buffer. Error: 0x{:X}", err);
    }
}

void AudioSource::Play() {
    ApplySettings();

    if (this->currentClip == nullptr) {
        spdlog::warn("AudioSource::Player: Cannot play, no AudioClip is assigned");
        return;
    }

    alGetError();

    alSourcePlay(sourceId);

    ALenum err = alGetError();
    if (err != AL_NO_ERROR) {
        spdlog::error("AudioSource::Play: OpenAL failed to play source. Error: 0x{:X}", err);
    }
}

void AudioSource::Stop() {
    alSourceStop(sourceId);
}

void AudioSource::Pause() {
    alSourcePause(sourceId);
}

void AudioSource::SetVolume(float volume) {
    this->volume = volume;
    alSourcef(sourceId, AL_GAIN, volume);
}

void AudioSource::SetPitch(float pitch) {
    this->pitch = pitch;
    alSourcef(sourceId, AL_PITCH, pitch);
}

void AudioSource::SetLooping(bool looping) {
    this->isLooping = looping;
    alSourcei(sourceId, AL_LOOPING, isLooping ? AL_TRUE : AL_FALSE);
}

ALuint AudioSource::GetSourceId() {
    return this->sourceId;
}

void AudioSource::DrawImGui() {
    float tempVolume = this->volume;
    if (ImGui::DragFloat("Volume", &tempVolume, 0.1f, 0.0f, 10.0f)) {
        SetVolume(tempVolume);
    }

    float tempPitch = this->pitch;
    if (ImGui::DragFloat("Pitch", &tempPitch, 0.01f, 0.1f, 3.0f)) {
        SetPitch(tempPitch);
    }

    bool tempLooping = this->isLooping;
    if (ImGui::Checkbox("Looping", &tempLooping)) {
        SetLooping(tempLooping);
    }

    bool temp2D = this->is2D;
    if (ImGui::Checkbox("Is 2D", &temp2D)) {
        Set2D(temp2D);
    }

    if (this->currentClip) {
        ImGui::TextWrapped("Clip: %s", this->currentClip->GetPath().string().c_str());
    } else {
        ImGui::Text("Clip: None assigned");
    }

    ImGui::Separator();

    if (ImGui::Button("Play")) {
        this->Play();
    }
    ImGui::SameLine();
    if (ImGui::Button("Pause")) {
        Pause();
    }
    ImGui::SameLine();
    if (ImGui::Button("Stop")) {
        Stop();
    }

    ALint state;
    alGetSourcei(sourceId, AL_SOURCE_STATE, &state);

    const char* stateString = "Unknown";
    if (state == AL_PLAYING) stateString = "Playing";
    else if (state == AL_PAUSED) stateString = "Paused";
    else if (state == AL_STOPPED) stateString = "Stopped";
    else if (state == AL_INITIAL) stateString = "Ready";

    ImGui::Text("Status: %s", stateString);
}
