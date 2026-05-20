#pragma once

#include "SceneComponent.h"
#include "EasingFunctions.h"

#include <optional>
#include <functional>
#include <vector>

class TweenSystem;
class TweenHandle;

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

    std::vector<std::function<void(float)>> setters;
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

  TweenHandle CreateTween(const TweenConfig config);

  float GetCurrentValue(const TweenId id) const;

  // If planning to share the handle outside of the owner, add a callt o RemoveTween in the destructor
  void RemoveTween(const TweenId id);

  bool IsValid(const TweenId id) const;

  void SetOnComplete(const TweenId id, const std::function<void()> onComplete);
  void SetPlaying(const TweenId id, const bool playing);
  void BindSetter(const TweenId id, std::function<void(float)> setter);

private:
  TweenSystem::Tween* GetTween(const TweenId id);
};

// This handle is meant to be stored inside of the gameobject whose values it's modifying, that prevents (somewhat) situations where the node stops being valid while the tween is still playing causing a crash
//  this isn't perfect since you can still bind it to another node's value and ideally should be replaced with some form of id's
class TweenHandle {
private:
    TweenSystem* system = nullptr;
    TweenId id = {0, 0};
public:
    TweenHandle() = default;
    TweenHandle(TweenSystem* system, TweenId id);
    
    TweenHandle(const TweenHandle&) = delete;
    TweenHandle& operator=(const TweenHandle&) = delete;

    TweenHandle(TweenHandle&& other) noexcept;

    TweenHandle& operator=(TweenHandle&& other) noexcept;

    ~TweenHandle();

    TweenHandle& Bind(std::function<void(float)> setter);
    TweenHandle& OnComplete(std::function<void()> callback);
    TweenHandle& SetPlaying(bool playing);

    float GetCurrentValue() const;

    // This is unsafe and should only be used if you can make sure the 
    //  node that we are binding the value to stays valid for the entire duration of the tween
    void Detach();
    
    operator TweenId() const;
};
