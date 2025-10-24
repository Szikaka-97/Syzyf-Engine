#include <Graphics.h>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <MeshRenderer.h>
#include <Camera.h>

#include <GLFW/glfw3.h>

SceneGraphics::SceneGraphics():
currentRenders(),
globalUniformsBuffer(0) {
	// currentRenders.reserve(64);
	// currentRenders.reserve()

	glGenBuffers(1, &this->globalUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(glm::mat4) * 3, nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);
}

void SceneGraphics::Render() {
	Camera* mainCamera = Camera::GetMainCamera();

	if (!mainCamera) {
		return;
	}
	
	ShaderGlobalUniforms globalUniforms;

	globalUniforms.Global_ViewMatrix = mainCamera->ViewMatrix();
	globalUniforms.Global_ProjectionMatrix = mainCamera->ProjectionMatrix();
	globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;
	globalUniforms.Global_Time = (float) glfwGetTime();

	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(globalUniforms), &globalUniforms);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	Material* currentMat = nullptr;
	Mesh currentMesh;

	int index = 0;

	ShaderObjectUniforms objectUniforms;

	for (auto node : this->currentRenders) {
	// for (RenderNode node = this->currentRenders[0]; node.nextIndex >= 0; node = this->currentRenders[node.nextIndex]) {
		Material* mat = node.renderer->GetMaterial();
		Mesh mesh = node.renderer->GetMesh();
	
		objectUniforms.Object_ModelMatrix = node.renderer->GlobalTransform();
		objectUniforms.Object_MVPMatrix = globalUniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;

		glBindBufferBase(GL_UNIFORM_BUFFER, 1, node.renderer->GetUniformBufferHandle());

		glBindBuffer(GL_UNIFORM_BUFFER, node.renderer->GetUniformBufferHandle());
		glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(objectUniforms), &objectUniforms);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		// if (mat != currentMat) {
			mat->Bind();
		// }
			
		// if (mesh.GetHandle() != currentMesh.GetHandle()) {
			glBindVertexArray(mesh.GetHandle());
		// }

		if (node.instanceCount <= 0) {
			glDrawElements(node.mode, mesh.GetTriangleCount() * 3, GL_UNSIGNED_INT, nullptr);
		}
		else {
			glDrawElementsInstanced(node.mode, mesh.GetTriangleCount() * 3, GL_UNSIGNED_INT, nullptr, node.instanceCount);
		}

		currentMat = mat;
		currentMesh = mesh;
	}

	glBindVertexArray(0);
	glUseProgram(0);

	this->currentRenders.clear();
}

void SceneGraphics::DrawMesh(MeshRenderer* renderer) {
	DrawMeshInstanced(renderer, 0);
}

void SceneGraphics::DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount) {
	// if (this->currentRenders.size() > 1) {
	// 	for (RenderNode& node = this->currentRenders[0]; node.nextIndex >= 0; node = this->currentRenders[node.nextIndex]) {
	// 		if (node.renderer->GetMaterial() == renderer->GetMaterial()) {
	// 			int newIndex = this->currentRenders.size();

	// 			this->currentRenders.push_back(RenderNode(
	// 				renderer,
	// 				GL_TRIANGLES,
	// 				node.nextIndex,
	// 				instanceCount
	// 			));

	// 			node.nextIndex = newIndex;
	// 		}
	// 	}
	// }
	// else {
		this->currentRenders.push_back(RenderNode(
			renderer,
			GL_TRIANGLES,
			-1,
			instanceCount
		));

		// if (this->currentRenders.size() == 2) {
		// 	this->currentRenders[0].nextIndex = 1;
		// }
	// }
}
