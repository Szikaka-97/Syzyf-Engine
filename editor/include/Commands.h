#include "CommandHistory.h"

#include <Scene.h>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <imgui.h>

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

    std::string GetName() const override { return "Transform"; }

    void ShowTooltip() const override {
        ImGui::BeginTooltip();
        ImGui::Text("Transform Action");
        ImGui::EndTooltip();
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

    std::string GetName() const override { return "Translate"; }

    void ShowTooltip() const override {
        ImGui::BeginTooltip();
        ImGui::Text("Translate Action");
        ImGui::Separator();
        ImGui::Text("Current/New Position: (%f, %f, %f)", newPosition.x,
                    newPosition.y, newPosition.z);
        ImGui::Text("Previous Position:    (%f, %f, %f)", oldPosition.x,
                    oldPosition.y, oldPosition.z);
        ImGui::EndTooltip();
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

    std::string GetName() const override { return "Rotate"; }

    void ShowTooltip() const override {
        ImGui::BeginTooltip();
        ImGui::Text("Translate Action");
        ImGui::Separator();
        ImGui::Text("Current/New Position: (%f, %f, %f)", newRotation.x,
                    newRotation.y, newRotation.z);
        ImGui::Text("Previous Position:    (%f, %f, %f)", oldRotation.x,
                    oldRotation.y, oldRotation.z);
        ImGui::EndTooltip();
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

    std::string GetName() const override { return "Scale"; }

    void ShowTooltip() const override {
        ImGui::BeginTooltip();
        ImGui::Text("Translate Action");
        ImGui::Separator();
        ImGui::Text("Current/New Position: (%f, %f, %f)", newScale.x,
                    newScale.y, newScale.z);
        ImGui::Text("Previous Position:    (%f, %f, %f)", oldScale.x,
                    oldScale.y, oldScale.z);
        ImGui::EndTooltip();
    }
};
} // namespace Editor
