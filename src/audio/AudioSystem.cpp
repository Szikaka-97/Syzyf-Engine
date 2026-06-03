#include "audio/AudioSystem.h"
#include "audio/AudioSource.h"
#include "Graphics.h"

#include <AL/al.h>

AudioSystem::AudioSystem(Scene* scene) : GameObjectSystem<AudioSource>(scene) {}

void AudioSystem::OnPostUpdate() {
    Camera* mainCamera = this->GetScene()->GetGraphics()->GetMainCamera();
    if (mainCamera != nullptr) {
        glm::vec3 cameraPosition = mainCamera->GlobalTransform().Position().Value();
        glm::vec3 cameraForward = mainCamera->GlobalTransform().Forward();
        glm::vec3 cameraUp = mainCamera->GlobalTransform().Up();

        alListener3f(AL_POSITION, cameraPosition.x, cameraPosition.y, cameraPosition.z);

        ALfloat listenerOrientation[] = { cameraForward.x, cameraForward.y, cameraForward.z, cameraUp.x, cameraUp.y, cameraUp.z };
        alListenerfv(AL_ORIENTATION, listenerOrientation);
    }

    for (AudioSource* source : IterateObjects()) {
        if (source->Is2D()) {
            continue;
        }

        glm::vec3 position = source->GlobalTransform().Position().Value();

        alSource3f(source->GetSourceId(), AL_POSITION, position.x, position.y, position.z);

        // add velocity maybe
    }
}
