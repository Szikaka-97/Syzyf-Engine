#include "TweenSystem.h"

#include "Scene.h"
#include <TimeSystem.h>

#include <spdlog/spdlog.h>

TweenSystem::TweenSystem(Scene* scene) : SceneComponent(scene) {
  this->allocator = Allocator();
}

void TweenSystem::OnPreUpdate() {
  const float deltaTime = Time::Delta();

  for (std::size_t i = 0; i < this->tweens.size(); ++i) {
    if (!this->tweens[i].has_value())
      continue;
    if (!this->tweens[i]->playing)
      continue;

    Tween& tween = this->tweens[i].value();
    tween.timeActive += deltaTime;

    if (tween.timeActive >= tween.tweenConfig.duration) {
      for (auto& value : tween.values) {
        *value = tween.tweenConfig.targetValue;
      }

      std::vector<std::function<void()>> cachedCallbacks = std::move(tween.onComplete);

      this->allocator.free.push_back(i);
      this->tweens[i].reset();

      for (auto& onComplete : cachedCallbacks) {
        onComplete();
      }

      continue;
    }

    for (auto& value : tween.values) {
      float difference = tween.tweenConfig.targetValue - tween.tweenConfig.initialValue;
      float progress = tween.timeActive / tween.tweenConfig.duration;
      float easingValue = tween.tweenConfig.easingFunction(progress); 
      float step = difference * easingValue;
      float newValue = tween.tweenConfig.initialValue + step;
      *value = newValue;
    }
  }
}

void TweenSystem::DrawImGui() {}

TweenId TweenSystem::CreateTween(const TweenConfig config) {
  TweenId id = this->allocator.Allocate();
  
  if (this->tweens.size() <= id.id)
    this->tweens.resize(id.id + 1);

  this->tweens[id.id].emplace(config);
  
  return id;
}

void TweenSystem::RemoveTween(const TweenId id) {
  if (this->tweens[id.id].has_value() && id.generation == this->allocator.generations[id.id]) {
    this->allocator.free.push_back(id.id);
    this->tweens[id.id].reset();
  }
}

bool TweenSystem::IsValid(const TweenId id) const {
  if (id.id >= this->tweens.size()) return false;
  return this->tweens[id.id].has_value() && id.generation == this->allocator.generations[id.id];
}

// Only values bound/objects captured by the lambda used should be the ones whose lifetimes
//  are at least as long as the tween, ideally the tween should only be used with the values of the owner
//  I can't easily make sure the values still are valid after the tween runs
// To achieve the same behaviour on another entity another tween with the same TweenConfig should be created
void TweenSystem::SetOnComplete(const TweenId id, const std::function<void()> onComplete) {
  if (!this->IsValid(id)) {
    spdlog::warn("TweenSystem: Tried setting an onComplete callback on an invalid handle");
    return;
  }
  if (onComplete == nullptr) {
    spdlog::warn("TweenSystem: Tried setting an invalid function as an 'onComplete' callback");
    return;
  }

  this->tweens[id.id]->onComplete.push_back(onComplete);
}

void TweenSystem::BindValue(const TweenId id, float* value) {
  if (!this->IsValid(id)) {
    spdlog::warn("TweenSystem: Tried binding a value on an invalid tween handle");
    return;
  }

  this->tweens[id.id]->values.push_back(value);
}

void TweenSystem::SetPlaying(const TweenId id, const bool playing) {
  if (!this->IsValid(id)) {
    spdlog::warn("TweenSystem: Tried setting the 'playing' variable on an invalid tween handle");
    return;
  }

  this->tweens[id.id]->playing = playing;
}

TweenSystem::Tween* TweenSystem::GetTween(const TweenId id) {
  if (IsValid(id)) {
    return &this->tweens[id.id].value();
  }

  return nullptr;
}

