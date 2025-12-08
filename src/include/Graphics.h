#pragma once

#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

struct ShaderGlobalUniforms;
class MeshRenderer;
class Scene;
class ComputeShaderDispatch;
class Texture2D;

// struct RenderBatch {
// 	Mesh* mesh;
// 	Material* material;
// 	int argsSize;
// };

class SceneGraphics {
	friend class Scene;
private:
	struct RenderNode {
		MeshRenderer* renderer;
		unsigned int mode;
		int nextIndex;
		int instanceCount;
	};
	
	Scene* scene;

	std::vector<RenderNode> currentRenders;
	GLuint globalUniformsBuffer;
	
	glm::vec2 screenResolution;
	
	GLuint lightsBuffer;

	GLuint depthPrepassFramebuffer;
	GLuint depthPrepassDepthTexture;

	GLuint colorPassFramebuffer;
	GLuint colorPassOutputTexture;

	SceneGraphics(Scene* scene);

	void RenderObjects(const ShaderGlobalUniforms& globalUniforms);
	void RenderFullscreenFrameQuad();

	void Render();
public:
	void UpdateScreenResolution(glm::vec2 newResolution);
	void DrawMesh(MeshRenderer* renderer);
	void DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount);
};

