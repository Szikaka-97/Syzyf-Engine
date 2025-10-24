#pragma once

#include <vector>
#include <glad/glad.h>

class MeshRenderer;
class Scene;

class SceneGraphics {
	friend class Scene;
private:
	struct RenderNode {
		MeshRenderer* renderer;
		unsigned int mode;
		int nextIndex;
		int instanceCount;

	};
	
	std::vector<RenderNode> currentRenders;
	GLuint globalUniformsBuffer;

	SceneGraphics();
	void Render();
public:
	void DrawMesh(MeshRenderer* renderer);
	void DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount);
};

