#include "animation/AnimationComponent.h"
#include <spdlog/spdlog.h>

AnimationComponent::AnimationComponent() {}

void AnimationComponent::Play(const std::string name) {
  for (auto& animation : this->animations) {
    if (animation.data.name == name) {
      animation.timeActive = 0.0f;
      animation.playing = true;
      return;
    }
  }
  spdlog::warn("AnimationComponent: Animation: {} not found on object: {}", name, this->GetNode()->GetName());
}
