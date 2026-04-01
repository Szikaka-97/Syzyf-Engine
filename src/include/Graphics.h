#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

#include <Camera.h>
#include <Framebuffer.h>
#include <GameObjectSystem.h>
#include <Layer.h>
#include <Material.h>
#include <Mesh.h>

struct ShaderGlobalUniforms;
class MeshRenderer;
class Scene;
class ComputeShaderDispatch;
class Texture2D;
class LightSystem;
class PostProcessingSystem;
class ReflectionProbeSystem;
class Camera;
class Viewport;

// struct RenderBatch {
// 	Mesh* mesh;
// 	Material* material;
// 	int argsSize;
// };

enum class RenderPassType {
  Color = 1,
  DepthPrepass = 2,
  Shadows = 6,
  Gizmos = 8,
  PostProcessing = 16,
  Transparent = 32,
  Volumetric = 64,
};

struct RenderParams {
  RenderPassType pass;
  glm::vec4 viewport;
  bool clearDepth;
  LayerMask layers;

  RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth = false,
               LayerMask layers = LayerMask::All);
};

class SceneGraphics : public GameObjectSystem<Camera> {
private:
  struct RenderNode {
    const Mesh::SubMesh *mesh;
    const Material *material;
    union {
      const unsigned int instanceCount;
      const bool ignoreDepth;
    };
    const glm::mat4 transformation;
    const BoundingBox bounds;
    uint8_t layer;

    int jointBufferOffset = -1;

    RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, uint8_t layer);
    RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer);
    RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, uint8_t layer);
    RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer);
	};

  std::vector<RenderNode> currentRenders;
  std::vector<RenderNode> gizmoRenders;
  std::vector<RenderNode> transparentRenders;
  std::vector<RenderNode> volumetricRenders;
  GLuint globalUniformsBuffer;
  GLuint objectUniformsBuffer;

  Viewport *mainViewport;
  Framebuffer *opaquePassFramebuffer;
  Framebuffer *transparentPassFramebuffer;
  Framebuffer *volumetricFramebuffer;

  LightSystem *lightSystem;
  PostProcessingSystem *postProcessing;
  ReflectionProbeSystem *envMapping;

  Camera *mainCamera;

  void RenderObjects(const ShaderGlobalUniforms &globalUniforms,
                     RenderParams params);
  void RenderFullscreenFrameQuad();
  void CompositeTransparentPass();

  void BindGlobalUniformBuffer(const ShaderGlobalUniforms &globalUniforms);

  void Render();

public:
  SceneGraphics(Scene *scene);

  glm::vec2 GetScreenResolution() const;
  void UpdateScreenResolution(glm::vec2 newResolution);

  LightSystem *GetLightSystem();
  PostProcessingSystem *GetPostProcessing();
  ReflectionProbeSystem *GetEnvMapping();

  Viewport *GetMainViewport() const;
  Framebuffer *GetMainFramebuffer() const;

  Camera *GetMainCamera() const;
  void SetMainCamera(Camera *camera);

  void DrawMesh(MeshRenderer *renderer);
  void DrawMesh(const Mesh *mesh, int subMeshIndex, const Material *material,
                const glm::mat4 &transformation,
                uint8_t layer = Layer::Default);
  void DrawMesh(const Mesh *mesh, int subMeshIndex, const Material *material,
                const glm::mat4 &transformation, const BoundingBox &bounds,
                uint8_t layer = Layer::Default);

  void DrawMeshInstanced(MeshRenderer *renderer, unsigned int instanceCount);
  void DrawMeshInstanced(const Mesh *mesh, int subMeshIndex,
                         const Material *material,
                         const glm::mat4 &transformation,
                         unsigned int instanceCount,
                         uint8_t layer = Layer::Default);
  void DrawMeshInstanced(const Mesh *mesh, int subMeshIndex,
                         const Material *material,
                         const glm::mat4 &transformation,
                         unsigned int instanceCount, const BoundingBox &bounds,
                         uint8_t layer = Layer::Default);

  void DrawGizmoMesh(const Mesh *mesh, int subMeshIndex,
                     const Material *material, const glm::mat4 &transformation,
                     bool ignoresDepth = false);

  void RenderCamera(Camera *camera, Viewport *renderTarget = nullptr);
  void RenderCamera(Camera *camera, const RenderParams &params);
  void RenderCamera(Camera *camera, Viewport *renderTarget,
                    const RenderParams &params);

  void RenderScene(const ShaderGlobalUniforms &uniforms,
                   Framebuffer *framebuffer, const RenderParams &params);
  void RenderScene(const CameraData &camera, Framebuffer *framebuffer,
                   const RenderParams &params);
  void RenderScene(Camera *camera, Framebuffer *framebuffer,
                   const RenderParams &params);

  void RenderScene(const ShaderGlobalUniforms &uniforms, Viewport *viewport,
                   const RenderParams &params);
  void RenderScene(const CameraData &camera, Viewport *viewport,
                   const RenderParams &params);
  void RenderScene(Camera *camera, Viewport *viewport,
                   const RenderParams &params);

  virtual void OnPostRender();

  virtual void DrawImGui();

  virtual int Order();
};

inline constexpr RenderPassType operator&(RenderPassType a, RenderPassType b) {
  return static_cast<RenderPassType>(static_cast<int>(a) & static_cast<int>(b));
}

inline constexpr RenderPassType operator|(RenderPassType a, RenderPassType b) {
	return static_cast<RenderPassType>(static_cast<int>(a) | static_cast<int>(b));
}
