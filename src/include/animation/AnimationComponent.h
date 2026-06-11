#pragma once

#include "Debug.h"
#include "GameObject.h"
#include <unordered_set>
#include <vector>
  
class AnimationComponent : public GameObject, public ImGuiDrawable {
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
    serialized std::string name = "";
    std::vector<Track> tracks;
    float duration = 0.0f;
  };

  struct Animation {
    // Required to update once after setting the time using SetTime or SetProgress
    bool isDirty = false;

    serialized fs::path source;
    serialized std::vector<SceneNode*> participants;
    serialized AnimationData data;

    // Bone masking/layers
    std::unordered_set<SceneNode*> boneMask;
    int layerIndex = 0;
    float blendWeight = 1.0f;

    float timeActive = 0.0f;
    // Per track 
    std::vector<size_t> currentKeyframes;

    serialized float speed = 1.0f;

    bool playing = false;
    bool looping = false;

    json Serialize() const;
    void Deserialize(const json& data);
  };

  serialized std::vector<Animation> animations;
public:
  AnimationComponent();

  // Plays the animation starting from the first frame
  // To pause/unpause use the `playing` member variable
  void Play(const std::string name);

  // Sets the animation progress to the specified time in seconds
  void SetTime(const std::string& name, float timeInSeconds);
  // Sets the animation progress to the specified percentage (0.0 - 1.0)
  void SetProgress(const std::string& name, float percent);

  // Sets which layer the animation plays on
  //    used for masks
  void SetAnimationLayer(const std::string& name, int layer);

  // Creates a mask starting from the `maskRoot`
  void SetAnimationMask(const std::string& nanme, SceneNode* maskRoot);

  void DrawImGui() override;

  virtual ~AnimationComponent() = default;
};
