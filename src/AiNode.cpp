#include <AiNode.h>

#include "Scene.h"
#include "TimeSystem.h"
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>
#include <Camera.h>
#include <Graphics.h>
#include <ImGui.h>


AiNode::AiNode()
    : m_Speed(5.0f)
    , m_RotationSpeed(2.0f)
    , m_TargetNode(nullptr) {
}

AiNode::~AiNode() {
}

void AiNode::Update() {
    if (!m_TargetNode) {
        Scene* scene = GetScene();
        if (scene) {
            SceneGraphics* graphics = scene->GetGraphics();
            if (graphics) {
                Camera* camera = graphics->GetMainCamera();
                if (camera) {
                    m_TargetNode = camera->GetNode();
                }
            }
        }
        if (!m_TargetNode) return;
    }

    SceneNode* myNode = GetNode();
    if (!myNode) return;

    glm::vec3 targetPos = m_TargetNode->GlobalTransform().Position();
    glm::vec3 myPos = myNode->GlobalTransform().Position();
    glm::vec3 direction = glm::normalize(targetPos - myPos);

    float deltaTime = Time::Delta();
    myNode->GlobalTransform().Position() += direction * m_Speed * deltaTime;

    glm::quat targetRotation = glm::quatLookAt(direction, glm::vec3(0, 1, 0));

    glm::quat currentRotation = myNode->GlobalTransform().Rotation();
    glm::quat newRotation = glm::slerp(currentRotation, targetRotation, m_RotationSpeed * deltaTime);
    myNode->GlobalTransform().Rotation() = newRotation;
}

void AiNode::SetTarget(SceneNode* target) {
    m_TargetNode = target;
}