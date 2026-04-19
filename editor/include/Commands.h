#include "CommandHistory.h"

#include <Scene.h>

namespace Editor {

class TransformCommand : public ICommand {
  private:
    SceneNode* target;
    glm::mat4 oldLocalTransform;
    glm::mat4 newLocalTransform;

  public:
    TransformCommand(SceneNode* target, glm::mat4 oldLocalTransform,
                     glm::mat4 newLocalTransform)
        : target(target), oldLocalTransform(oldLocalTransform),
          newLocalTransform(newLocalTransform) {}

    void Execute() override {
        this->target->LocalTransform() = this->newLocalTransform;
    }

    void Undo() override {
        this->target->LocalTransform() = this->oldLocalTransform;
    }
};

class TranslateCommand : public ICommand {
  private:
    SceneNode* target;
    glm::vec3 oldPosition;
    glm::vec3 newPosition;

  public:
    TranslateCommand(SceneNode* target, glm::vec3 oldPosition,
                     glm::vec3 newPosition)
        : target(target), oldPosition(oldPosition), newPosition(newPosition) {}

    void Execute() override {
        this->target->GlobalTransform().Position() = this->newPosition;
    }

    void Undo() override {
        this->target->GlobalTransform().Position() = this->oldPosition;
    }
};

class RotateCommand : public ICommand {
  private:
    SceneNode* target;
    glm::quat oldRotation;
    glm::quat newRotation;

  public:
    RotateCommand(SceneNode* target, glm::quat oldRotation,
                  glm::quat newRotation)
        : target(target), oldRotation(oldRotation), newRotation(newRotation) {}

    void Execute() override {
        this->target->GlobalTransform().Rotation() = this->newRotation;
    }

    void Undo() override {
        this->target->GlobalTransform().Rotation() = this->oldRotation;
    }
};

class ScaleCommand : public ICommand {
  private:
    SceneNode* target;
    glm::vec3 oldScale;
    glm::vec3 newScale;

  public:
    ScaleCommand(SceneNode* target, glm::vec3 oldScale, glm::vec3 newScale)
        : target(target), oldScale(oldScale), newScale(newScale) {}

    void Execute() override {
        this->target->GlobalTransform().Scale() = this->newScale;
    }

    void Undo() override {
        this->target->GlobalTransform().Scale() = this->oldScale;
    }
};
} // namespace Editor
