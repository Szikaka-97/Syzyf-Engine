#include "animation/AnimationComponent.h"

#include <spdlog/spdlog.h>
#include <imgui.h>

#include <GltfScene.h>

AnimationComponent::AnimationComponent() {}

void AnimationComponent::Play(const std::string name) {
  for (auto& animation : this->animations) {
    if (animation.data.name == name) {
      animation.timeActive = 0.0f;
      animation.playing = true;

      animation.currentKeyframes.assign(animation.data.tracks.size(), 0);
      return;
    }
  }
  spdlog::warn("AnimationComponent: Animation: {} not found on object: {}", name, this->GetNode()->GetName());
}

void AnimationComponent::SetTime(const std::string& name, float timeInSeconds) {
    for (auto& animation : this->animations) {
        if (animation.data.name == name) {
            animation.timeActive = std::clamp(timeInSeconds, 0.0f, animation.data.duration);

            animation.currentKeyframes.assign(animation.data.tracks.size(), 0);
            animation.isDirty = true;
            return;
        }
    }
}

void AnimationComponent::SetProgress(const std::string& name, float percent) {
    for (auto& animation : this->animations) {
        if (animation.data.name == name) {
            float targetTime = animation.data.duration * std::clamp(percent, 0.0f, 1.0f);
            SetTime(name, targetTime);
            return;
        }
    }
}

void AnimationComponent::DrawImGui() {
    for (auto& animation : this->animations) {
        if (ImGui::TreeNode(animation.data.name.c_str())) {
            ImGui::Checkbox("Playing", &animation.playing);
            ImGui::SameLine();
            ImGui::Checkbox("Looping", &animation.looping);

            ImGui::DragFloat("Speed", &animation.speed, 0.1f, -5.0f, 5.0f);

            float scrubTime = animation.timeActive;
            if (ImGui::SliderFloat("Timeline", &scrubTime, 0.0f, animation.data.duration, "%.3f s")) {
                SetTime(animation.data.name, scrubTime);
            }

            ImGui::TreePop();
        }
    }
}

json AnimationComponent::Animation::Serialize() const {
    return json{};
}

void AnimationComponent::Animation::Deserialize(const json& data) {
    GltfScene* sourceScene = ResourceDatabase::Global->Get<GltfScene>(this->source);

    sourceScene->GetAnimationData(*this);
}