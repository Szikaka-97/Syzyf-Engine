#pragma once

#include "SceneComponent.h"
#include "EasingFunctions.h"

#include <optional>
#include <functional>
#include <vector>

class TweenSystem;

struct TweenConfig {
  float initialValue = 0.0f;
  float targetValue = 0.0f;
  float duration = 0.0f;

  std::function<float(float)> easingFunction = Easing::inOutSine;
};

struct TweenId {
  std::size_t id;
  std::size_t generation;
};

class TweenSystem : public SceneComponent {
private:
  struct Tween {
    float timeActive = 0.0f;
    bool playing = true;

    std::vector<float*> values;
    std::vector<std::function<void()>> onComplete;

    TweenConfig tweenConfig = {};

    Tween(TweenConfig config) : tweenConfig(config) {}
  };

  class Allocator {
  public:
    std::vector<std::size_t> free;
    std::vector<std::size_t> generations;
   
  public:
    Allocator() {};

    TweenId Allocate() {
      if (this->free.empty()) {
        this->generations.push_back(0);
        return TweenId {
          this->generations.size() - 1,
          0,
        };
      };

      std::size_t id = this->free.back();
      this->free.pop_back();
      this->generations[id] += 1;
      return TweenId {
        id,
        this->generations[id],
      };
    }
  };

  Allocator allocator;
  std::vector<std::optional<Tween>> tweens;

public:
  TweenSystem(Scene* scene);

  void OnPreUpdate();
  void DrawImGui();

  TweenId CreateTween(const TweenConfig config);

  // If planning to share the handle outside of the owner, add a callt o RemoveTween in the destructor
  void RemoveTween(const TweenId id);

  bool IsValid(const TweenId id) const;

  void SetOnComplete(const TweenId id, const std::function<void()> onComplete);
  void SetPlaying(const TweenId id, const bool paused);
  void BindValue(const TweenId id, float* value);

private:
  TweenSystem::Tween* GetTween(const TweenId id);
};
