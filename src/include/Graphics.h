#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <Mesh.h>
#include <Material.h>
#include <Camera.h>
#include <Framebuffer.h>
#include <SceneComponent.h>

struct ShaderGlobalUniforms;
class MeshRenderer;
class Scene;
class ComputeShaderDispatch;
class Texture2D;
class Light;

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
	PostProcessing = 16
};

struct RenderParams {
	RenderPassType pass;
	glm::vec4 viewport;
	bool clearDepth;

	RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth = false);
};

class SceneGraphics : public SceneComponent {
	struct RenderNode {
		const Mesh::SubMesh* mesh;
		const Material* material;
		const unsigned int instanceCount;
		const glm::mat4 transformation;
	};

	std::vector<RenderNode> currentRenders;
	GLuint globalUniformsBuffer;
	GLuint objectUniformsBuffer;
	
	glm::vec2 screenResolution;
	
	Framebuffer* depthPrepassFramebuffer;
	Texture2D* depthPrepassDepthTexture;

	Framebuffer* colorPassFramebuffer;
	Texture2D* colorPassOutputTexture;
	
	void RenderObjects(const ShaderGlobalUniforms& globalUniforms, RenderParams params);
	void RenderFullscreenFrameQuad();
	
	void BindGlobalUniformBuffer(const ShaderGlobalUniforms& globalUniforms);
	
	void Render();
public:
	SceneGraphics(Scene* scene);

	glm::vec2 GetScreenResolution() const;
	void UpdateScreenResolution(glm::vec2 newResolution);
	
	void DrawMesh(MeshRenderer* renderer);
	void DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation);
	
	void DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount);
	void DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount);

	void RenderScene(const ShaderGlobalUniforms& uniforms, Framebuffer* framebuffer, const RenderParams& params);
	void RenderScene(const CameraData& camera, Framebuffer* framebuffer, const RenderParams& params);
	void RenderScene(Camera* camera, Framebuffer* framebuffer, const RenderParams& params);

	virtual void OnPostRender();

	virtual int Order();
};
