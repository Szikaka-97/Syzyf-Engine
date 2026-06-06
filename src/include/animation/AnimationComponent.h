#pragma once

#include "Debug.h"
#include "GameObject.h"
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

  void DrawImGui() override;

  virtual ~AnimationComponent() = default;
};
