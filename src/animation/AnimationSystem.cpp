#include "animation/AnimationSystem.h"
#include "animation/AnimationComponent.h"
#include "animation/SkeletonSystem.h"

#include "TimeSystem.h"
#include "Scene.h"

#include <glm/ext/quaternion_common.hpp>

glm::quat cubicSpline(const glm::quat previousPoint, const glm::quat previousTangent, const glm::quat nextPoint, const glm::quat nextTangent, const float interpolationValue) {
  const float t = interpolationValue;
  const float t2 = t * t;
  const float t3 = t2 * t;

  return (2.0f * t3 - 3.0f * t2 + 1.0f)
    * previousPoint + (t3 - 2.0f * t2 + t)
    * previousTangent + (-2.0f * t3 + 3.0f * t2)
    * nextPoint + (t3 - t2) * nextTangent;
}

glm::vec3 cubicSpline(const glm::vec3 previousPoint, const glm::vec3 previousTangent, const glm::vec3 nextPoint, const glm::vec3 nextTangent, const float interpolationValue) {
  const float t = interpolationValue;
  const float t2 = t * t;
  const float t3 = t2 * t;

  return (2.0f * t3 - 3.0f * t2 + 1.0f)
    * previousPoint + (t3 - 2.0f * t2 + t)
    * previousTangent + (-2.0f * t3 + 3.0f * t2)
    * nextPoint + (t3 - t2) * nextTangent;
}

AnimationSystem::AnimationSystem(Scene* scene): GameObjectSystem<AnimationComponent>(scene) {
  scene->AddComponent<SkeletonSystem>();
  spdlog::info("Animation system added");
}

void AnimationSystem::OnPreUpdate() {
  const float deltaTime = Time::Delta(); 

  for (auto* object : IterateObjects()) {
    for (auto& animation : object->animations) {
      if (!animation.playing && !animation.isDirty)
        continue;

      if (animation.playing) {
          animation.timeActive += deltaTime * animation.speed;

          spdlog::debug("Animation: {}, duration: {}, timeactive: {}", animation.data.name, animation.data.duration, animation.timeActive);
          if (animation.timeActive >= animation.data.duration) {
            for (std::size_t& i : animation.currentKeyframes) {
              i = 0;
            }
            if (animation.looping) {
              animation.timeActive = std::fmod(animation.timeActive, animation.data.duration);
            } else {
              animation.playing = false;
              animation.timeActive = animation.data.duration;
            }
          }
      }

      animation.isDirty = false;

      for (std::size_t i = 0; i < animation.data.tracks.size(); ++i) {
        auto& track = animation.data.tracks[i];

        std::size_t lowerIndex = 0;
        std::size_t upperIndex = 0;
        if (animation.timeActive < track.inputs.front()) {
          lowerIndex = 0;
          upperIndex = 0;
        } else if (animation.timeActive >= track.inputs.back()) {
          lowerIndex = track.inputs.size() - 1;
          upperIndex = lowerIndex;
        } else {
          while (animation.currentKeyframes[i] < track.inputs.size() - 1
            && animation.timeActive >= track.inputs[animation.currentKeyframes[i] + 1]) {
            animation.currentKeyframes[i] += 1;
          }

          lowerIndex = animation.currentKeyframes[i];
          upperIndex = animation.currentKeyframes[i] + 1;
        }

        switch (track.property) {
          case AnimationComponent::Property::ROTATION:
            {
              glm::quat rotation;

              glm::quat lower = {
                track.outputs[lowerIndex * 4 + 3],
                track.outputs[lowerIndex * 4],
                track.outputs[lowerIndex * 4 + 1],
                track.outputs[lowerIndex * 4 + 2],
              };

              // Skips interpolation if the track hasn't started yet/ended already 
              if (upperIndex == lowerIndex) {
                rotation = lower;
                track.target->LocalTransform().Rotation() = rotation;
                break;
              }

              const float keyFrameDuration = track.inputs[upperIndex] - track.inputs[lowerIndex];
              const float keyFrameTimeActive = animation.timeActive - track.inputs[lowerIndex];
              const float interpolationValue = keyFrameTimeActive / keyFrameDuration;

              glm::quat upper = {
                track.outputs[upperIndex * 4 + 3],
                track.outputs[upperIndex * 4],
                track.outputs[upperIndex * 4 + 1],
                track.outputs[upperIndex * 4 + 2],
              };
 
              switch (track.interpolation) {
                case AnimationComponent::Interpolation::LINEAR:
                  rotation = glm::slerp(lower, upper, interpolationValue);
                  break;
                case AnimationComponent::Interpolation::STEP:
                  rotation = lower;
                  break;
                case AnimationComponent::Interpolation::CUBICSPLINE:
                  glm::quat lowerValue = {
                    track.outputs[lowerIndex * 4 + 7],
                    track.outputs[lowerIndex * 4 + 4],
                    track.outputs[lowerIndex * 4 + 5],
                    track.outputs[lowerIndex * 4 + 6],
                  };
                  glm::quat lowerOutputTangent = {
                    track.outputs[lowerIndex * 4 + 11],
                    track.outputs[lowerIndex * 4 + 8],
                    track.outputs[lowerIndex * 4 + 9],
                    track.outputs[lowerIndex * 4 + 10],
                  };

                  glm::quat upperInputTangent = upper;
                  glm::quat upperValue = {
                    track.outputs[upperIndex * 4 + 7],
                    track.outputs[upperIndex * 4 + 4],
                    track.outputs[upperIndex * 4 + 5],
                    track.outputs[upperIndex * 4 + 6],
                  };

                  glm::quat previousTangent = keyFrameDuration * lowerOutputTangent;
                  glm::quat nextTangent = keyFrameDuration * upperInputTangent;

                  rotation = glm::normalize(cubicSpline(lowerValue, previousTangent, upperValue, nextTangent, interpolationValue));
                  break;
                }
              track.target->LocalTransform().Rotation() = rotation;
              break;
            }
          case AnimationComponent::Property::POSITION:
            {
              glm::vec3 position;

              glm::vec3 lower = {
                track.outputs[lowerIndex * 3],
                track.outputs[lowerIndex * 3 + 1],
                track.outputs[lowerIndex * 3 + 2],
              };

              if (upperIndex == lowerIndex) {
                position = lower;
                track.target->LocalTransform().Position() = position;
                break;
              }

              const float keyFrameDuration = track.inputs[upperIndex] - track.inputs[lowerIndex];
              const float keyFrameTimeActive = animation.timeActive - track.inputs[lowerIndex];
              const float interpolationValue = keyFrameTimeActive / keyFrameDuration;

              glm::vec3 upper = {
                track.outputs[upperIndex * 3],
                track.outputs[upperIndex * 3 + 1],
                track.outputs[upperIndex * 3 + 2],
              };
 
              switch (track.interpolation) {
                case AnimationComponent::Interpolation::LINEAR:
                  position = glm::mix(lower, upper, interpolationValue);
                  break;
                case AnimationComponent::Interpolation::STEP:
                  position = lower;
                  break;
                case AnimationComponent::Interpolation::CUBICSPLINE:
                  glm::vec3 lowerValue = {
                    track.outputs[lowerIndex * 3 + 3],
                    track.outputs[lowerIndex * 3 + 4],
                    track.outputs[lowerIndex * 3 + 5],
                  };
                  glm::vec3 lowerOutputTangent = {
                    track.outputs[lowerIndex * 3 + 6],
                    track.outputs[lowerIndex * 3 + 7],
                    track.outputs[lowerIndex * 3 + 8],
                  };

                  glm::vec3 upperInputTangent = upper;
                  glm::vec3 upperValue = {
                    track.outputs[upperIndex * 3 + 3],
                    track.outputs[upperIndex * 3 + 4],
                    track.outputs[upperIndex * 3 + 5],
                  };

                  glm::vec3 previousTangent = keyFrameDuration * lowerOutputTangent;
                  glm::vec3 nextTangent = keyFrameDuration * upperInputTangent;

                  position = cubicSpline(lowerValue, previousTangent, upperValue, nextTangent, interpolationValue);
                  break;
                }
              track.target->LocalTransform().Position() = position;
              break;
            }
          case AnimationComponent::Property::SCALE:
            {
              glm::vec3 scale;

              glm::vec3 lower = {
                track.outputs[lowerIndex * 3],
                track.outputs[lowerIndex * 3 + 1],
                track.outputs[lowerIndex * 3 + 2],
              };

              if (upperIndex == lowerIndex) {
                scale = lower;
                track.target->LocalTransform().Scale() = scale;
                break;
              }

              const float keyFrameDuration = track.inputs[upperIndex] - track.inputs[lowerIndex];
              const float keyFrameTimeActive = animation.timeActive - track.inputs[lowerIndex];
              const float interpolationValue = keyFrameTimeActive / keyFrameDuration;

              glm::vec3 upper = {
                track.outputs[upperIndex * 3],
                track.outputs[upperIndex * 3 + 1],
                track.outputs[upperIndex * 3 + 2],
              };
 
              switch (track.interpolation) {
                case AnimationComponent::Interpolation::LINEAR:
                  scale = glm::mix(lower, upper, interpolationValue);
                  break;
                case AnimationComponent::Interpolation::STEP:
                  scale = lower;
                  break;
                case AnimationComponent::Interpolation::CUBICSPLINE:
                  glm::vec3 lowerValue = {
                    track.outputs[lowerIndex * 3 + 3],
                    track.outputs[lowerIndex * 3 + 4],
                    track.outputs[lowerIndex * 3 + 5],
                  };
                  glm::vec3 lowerOutputTangent = {
                    track.outputs[lowerIndex * 3 + 6],
                    track.outputs[lowerIndex * 3 + 7],
                    track.outputs[lowerIndex * 3 + 8],
                  };

                  glm::vec3 upperInputTangent = upper;
                  glm::vec3 upperValue = {
                    track.outputs[upperIndex * 3 + 3],
                    track.outputs[upperIndex * 3 + 4],
                    track.outputs[upperIndex * 3 + 5],
                  };

                  glm::vec3 previousTangent = keyFrameDuration * lowerOutputTangent;
                  glm::vec3 nextTangent = keyFrameDuration * upperInputTangent;

                  scale = cubicSpline(lowerValue, previousTangent, upperValue, nextTangent, interpolationValue);
                  break;
                }
              track.target->LocalTransform().Scale() = scale;
              break;
            }
          case AnimationComponent::Property::WEIGHTS:
            // TODO
            break;
        }
      }
    }
  }
};

void AnimationSystem::UnregisterObjectForced(GameObject* obj) {
  //TODO: Make it a bit more efficient

  for (auto* object : IterateObjects()) {
    for (auto& animation : object->animations) {
      std::erase_if(animation.data.tracks, [obj](const auto& track) -> bool {
        return track.target == obj->GetNode();
      });
    }
  }
}
