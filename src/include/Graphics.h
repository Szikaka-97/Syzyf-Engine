#pragma once

#include <queue>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <Mesh.h>
#include <Material.h>
#include <Camera.h>
#include <Framebuffer.h>
#include <GameObjectSystem.h>
#include <Layer.h>

#include "../res/shaders/shared/uniforms.h"

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
	Additive = 64,
	Volumetric = 128,
};

struct RenderParams {
	RenderPassType pass;
	glm::vec4 viewport;
	bool clearDepth;
	LayerMask layers;

	RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth = false, LayerMask layers = LayerMask::All);
};

class SceneGraphics : public GameObjectSystem<Camera> {
private:
	struct RenderNode {
		const Mesh::SubMesh* mesh;
		const Material* material;
		union {
			unsigned int instanceCount;
			bool ignoreDepth;
		};
		glm::mat4 transformation;
		BoundingBox bounds;
		uint8_t layer;

		RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, uint8_t layer);
		RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer);
		RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, uint8_t layer);
		RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer);

		bool operator<(const RenderNode& other) const;
	};

	std::vector<RenderNode> opaqueRenders;
	std::vector<RenderNode> gizmoRenders;
	std::vector<RenderNode> transparentRenders;
	std::vector<RenderNode> oitTransparentRenders;
	std::vector<RenderNode> additiveRenders;
	std::vector<RenderNode> volumetricRenders;
	GLuint globalUniformsBuffer;
	GLuint objectUniformsBuffer;
	
	Viewport* mainViewport;
	Framebuffer* opaquePassFramebuffer;
	Framebuffer* transparentPassFramebuffer;
	Framebuffer* volumetricPassFramebuffer;
    float depthMult = 1.0f;

	LightSystem* lightSystem;
	PostProcessingSystem* postProcessing;
	ReflectionProbeSystem* envMapping;

	Camera* mainCamera;

	ShaderGlobalUniforms currentUniforms;
	ShaderProgram* depthOnlyShader;

	void RenderFullscreenFrameQuad();
	void CompositeTransparentPass();
	void CompositeVolumetricPass();
	
	void RenderPostprocess();

	void Render();

	void EnqueueOpaque(const RenderNode& node);
	void EnqueueGizmo(const RenderNode& node);
	void EnqueueOrderedTransparent(const RenderNode& node);
	void EnqueueOITransparent(const RenderNode& node);
	void EnqueueAdditive(const RenderNode& node);
	void EnqueueVolumetric(const RenderNode& node);

	void BindMaterialProperties(Material* mat);
public:
	SceneGraphics(Scene* scene);
	
	glm::vec2 GetScreenResolution() const;
	void UpdateScreenResolution(glm::vec2 newResolution);
	
	LightSystem* GetLightSystem();
	PostProcessingSystem* GetPostProcessing();
	ReflectionProbeSystem* GetEnvMapping();
	
	Viewport* GetMainViewport() const;
	Framebuffer* GetMainFramebuffer() const;
	
	Camera* GetMainCamera() const;
	void SetMainCamera(Camera* camera);

	void BindUniformBuffers();
	
	void DrawMesh(MeshRenderer* renderer);
	void DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, uint8_t layer = Layer::Default);
	void DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer = Layer::Default);
	
	void DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount);
	void DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, uint8_t layer = Layer::Default);
	void DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, const BoundingBox& bounds, uint8_t layer = Layer::Default);
	
	void DrawGizmoMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, bool ignoresDepth = false);
	
	void RenderCamera(Camera* camera, Viewport* renderTarget = nullptr);
	void RenderCamera(Camera* camera, const RenderParams& params);
	void RenderCamera(Camera* camera, Viewport* renderTarget, const RenderParams& params);
	
	void RenderPrepass(const RenderParams& params, Framebuffer* target);
	void RenderPrepass(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderOpaque(const RenderParams& params, Framebuffer* target);
	void RenderOpaque(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderOrderedTransparent(const RenderParams& params, Framebuffer* target);
	void RenderOrderedTransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderOITransparent(const RenderParams& params, Framebuffer* target);
	void RenderOITransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderAdditive(const RenderParams& params, Framebuffer* target);
	void RenderAdditive(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderGizmos(const RenderParams& params, Framebuffer* target);
	void RenderGizmos(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderShadows(const RenderParams& params, Framebuffer* target);
	void RenderShadows(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);
	
	void RenderVolumetric(const RenderParams& params, Framebuffer* target);
	void RenderVolumetric(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

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
