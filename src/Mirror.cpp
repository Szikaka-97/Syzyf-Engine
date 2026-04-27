#include "Mirror.h"

#include "Camera.h"
#include "Shader.h"
#include "Material.h"
#include "Framebuffer.h"
#include "Graphics.h"
#include "MeshRenderer.h"

#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_inverse.hpp>

Mirror::Mirror(Mesh* mesh) {
  SceneNode* rootNode = this->GetScene()->CreateNode(this->GetNode());
  this->cameraNode = this->GetScene()->CreateNode(rootNode);
  cameraNode->AddObject<Camera>(Camera::Perspective(
    25.0f, 16.0f/9.0f, 0.1f, 200.0f));
  
  this->viewport = Viewport();
  this->viewport.GetFramebuffer()->CreateColorAttachment(true, false);
  this->viewport.GetFramebuffer()->CreateDepthAttachment(true, false);
  this->viewport.SetSize(glm::uvec2(1920, 1080));

  this->cameraNode->GetObject<Camera>()->SetAspectRatio(16.0f / 9.0f);
  this->cameraNode->GetObject<Camera>()->SetRenderTarget(&this->viewport);

  uint32_t hiddenLayer = 1;
  uint32_t mask = ~0 & ~(1 << hiddenLayer);
  this->cameraNode->GetObject<Camera>()->SetLayerMask(mask);

  ShaderProgram* shaderProgram = ShaderProgram::Build().WithVertexShader(
    this->GetScene()->Resources()->Get<VertexShader>("./res/shaders/mirror/mirror.vert")
  ).WithPixelShader(
    this->GetScene()->Resources()->Get<PixelShader>("./res/shaders/mirror/mirror.frag")
  ).Link();

  this->material = new Material(shaderProgram);
  this->material->SetValue("uColor", glm::vec3(1, 1, 1));
  this->material->SetValue("colorTex", (Texture2D*) this->viewport.GetFramebuffer()->GetColorTexture());

  auto* mirrorMesh = this->GetScene()->CreateNode(this->GetNode(), "Mirror Mesh");
  mirrorMesh->AddObject<MeshRenderer>(mesh, this->material);
  mirrorMesh->SetLayer(1);
}

void Mirror::Update() {
  if (this->playerNode == nullptr) {
    this->playerNode = this->GetScene()->GetGraphics()->GetMainCamera()->GetNode(); // xd
    if (this->playerNode != nullptr) {
      spdlog::info("Mirror: Found main camera node");
    } else {
      spdlog::warn("Mirror: main camera node is missing");
      return;
    }
  }

  // shouldnt happen each frame probably, or at all
  //  leaving it in right now because the editor viewport doesnt have a set resolution
  Camera* mainCamera = this->playerNode->GetObject<Camera>();
  Camera* mirrorCamera = this->cameraNode->GetObject<Camera>();

  if (mainCamera && mirrorCamera) {
      mirrorCamera->SetAspectRatio(mainCamera->GetAspectRatio());
      glm::uvec2 resolution = this->GetScene()->GetGraphics()->GetMainFramebuffer()->GetSize();
      // resolution.x = resolution.x * 0.5f;
      // resolution.y = resolution.y * 0.5f;
      this->viewport.SetSize(resolution);

      mirrorCamera->SetFov(mainCamera->GetFov());
  }

  const auto& playerCameraTransform = this->playerNode->GlobalTransform().Value();
  const auto& transform = this->GlobalTransform().Value();
  auto relativeTransform = glm::inverse(transform) * playerCameraTransform;
  relativeTransform[3][2] = -relativeTransform[3][2]; // Z translation

  auto cameraPosition = glm::vec3(relativeTransform[3]);

  // -transform.basis.z
  auto localForward = -glm::vec3(
    relativeTransform[2][0],
    relativeTransform[2][1],
    relativeTransform[2][2]
  );
  // transform.basis.y
  auto localUp = glm::vec3(
    relativeTransform[1][0],
    relativeTransform[1][1],
    relativeTransform[1][2]
  );

  localForward.z = -localForward.z;
  localUp.z = -localUp.z;

  auto viewMatrix = glm::lookAt(glm::vec3(0.0f), localForward, localUp);
  glm::mat4 reflectedTransform = glm::mat3(glm::affineInverse(viewMatrix)); 
  reflectedTransform[3] = glm::vec4(cameraPosition, 1.0f);

  this->cameraNode->GlobalTransform() = transform * reflectedTransform;
}
