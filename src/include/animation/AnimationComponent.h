#pragma once

#include "GameObject.h"
#include <vector>
  
class AnimationComponent : public GameObject {
public:
  enum Property {
    POSITION,
    SCALE,
    ROTATION,
    WEIGHTS
  };

  enum Interpolation {
    LINEAR,
    STEP,
    CUBICSPLINE
  };

  struct Track {
    SceneNode* target = nullptr;
    Property property;
    Interpolation interpolation;
    std::vector<float> inputs;
    std::vector<float> outputs;
  };

  struct AnimationData {
    std::string name = "";
    std::vector<Track> tracks;
    float duration = 0.0f;
  };

  struct Animation {
    AnimationData data;    

    float timeActive = 0.0f;
    // Per track 
    std::vector<size_t> currentKeyframes;

    float speed = 1.0f;

    bool playing = false;
    bool looping = false;
  };

  std::vector<Animation> animations;
public:
  AnimationComponent();

  void Play(const std::string name);

  virtual ~AnimationComponent() = default;
};
