#pragma once

#include "GameObjectSystem.h"
#include "AudioSource.h"

class AudioSystem : public GameObjectSystem<AudioSource> {
public:
    AudioSystem(Scene* scene);
    virtual ~AudioSystem() = default;

    virtual void OnPostUpdate() override;
};
