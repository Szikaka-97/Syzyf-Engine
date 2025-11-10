#include <Graphics.h>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <MeshRenderer.h>
#include <Camera.h>
#include <Skybox.h>
#include <Resources.h>

#include <GLFW/glfw3.h>

SceneGraphics::SceneGraphics():
currentRenders(),
globalUniformsBuffer(0),
depthPrepassFramebuffer(0),
depthPrepassDepthTexture(0),
colorPassFramebuffer(0),
colorPassOutputTexture(0) {
	// currentRenders.reserve(64);
	// currentRenders.reserve()

	glGenBuffers(1, &this->globalUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderGlobalUniforms), nullptr, GL_STATIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);
}

void SceneGraphics::UpdateScreenResolution(glm::vec2 newResolution) {
	if (this->screenResolution != newResolution) {
		this->screenResolution = newResolution;

		if (!this->depthPrepassDepthTexture) {
			glGenTextures(1, &this->depthPrepassDepthTexture);
			glBindTexture(GL_TEXTURE_2D, this->depthPrepassDepthTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
		}

		if (!this->colorPassOutputTexture) {
			glGenTextures(1, &this->colorPassOutputTexture);
			glBindTexture(GL_TEXTURE_2D, this->colorPassOutputTexture);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
		
		glBindTexture(GL_TEXTURE_2D, this->depthPrepassDepthTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, newResolution.x, newResolution.y, 0,  GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		
		glBindTexture(GL_TEXTURE_2D, this->colorPassOutputTexture);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, newResolution.x, newResolution.y, 0,  GL_RGBA, GL_UNSIGNED_BYTE, nullptr);		

		glBindTexture(GL_TEXTURE_2D, 0);

		if (!this->depthPrepassFramebuffer) {
			glGenFramebuffers(1, &this->depthPrepassFramebuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, this->depthPrepassFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthPrepassDepthTexture, 0);
		}
		if (!this->colorPassFramebuffer) {
			glGenFramebuffers(1, &this->colorPassFramebuffer);

			glBindFramebuffer(GL_FRAMEBUFFER, this->colorPassFramebuffer);

			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, this->colorPassOutputTexture, 0);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->depthPrepassDepthTexture, 0);
		}
	}
}

void SceneGraphics::RenderObjects(const ShaderGlobalUniforms& globalUniforms) {
	Material* currentMat = nullptr;
	Mesh* currentMesh;

	int index = 0;

	ShaderObjectUniforms objectUniforms;

	for (auto node : this->currentRenders) {
		Material* mat = node.renderer->GetMaterial();
		Mesh* mesh = node.renderer->GetMesh();
	
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
			glBindVertexArray(mesh->GetHandle());
		// }

		if (node.instanceCount <= 0) {
			glDrawElements(node.mode, mesh->GetTriangleCount() * 3, GL_UNSIGNED_INT, nullptr);
		}
		else {
			glDrawElementsInstanced(node.mode, mesh->GetTriangleCount() * 3, GL_UNSIGNED_INT, nullptr, node.instanceCount);
		}

		currentMat = mat;
		currentMesh = mesh;
	}
}

void SceneGraphics::Render() {
	Camera* mainCamera = Camera::GetMainCamera();

	if (!mainCamera) {
		return;
	}
		
	glViewport(0, 0, this->screenResolution.x, this->screenResolution.y);
	
	ShaderGlobalUniforms globalUniforms;
	globalUniforms.Global_ViewMatrix = mainCamera->ViewMatrix();
	globalUniforms.Global_ProjectionMatrix = mainCamera->ProjectionMatrix();
	globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;
	globalUniforms.Global_Time = (float) glfwGetTime();

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(globalUniforms), &globalUniforms);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, this->depthPrepassFramebuffer);

	glClear(GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);

	RenderObjects(globalUniforms);

	glBindFramebuffer(GL_FRAMEBUFFER, this->colorPassFramebuffer);

	glDepthFunc(GL_LEQUAL);

	RenderObjects(globalUniforms);

	Skybox* sky = Skybox::GetCurrentSkybox();

	if (sky) {
		sky->GetSKyMaterial()->Bind();
		glBindVertexArray(sky->GetSkyMesh()->GetHandle());
		glDrawElements(GL_TRIANGLES, sky->GetSkyMesh()->GetTriangleCount() * 3, GL_UNSIGNED_INT, nullptr);
	}

	RenderFullscreenFrameQuad();

	glBindVertexArray(0);
	glUseProgram(0);

	this->currentRenders.clear();
}

void SceneGraphics::RenderFullscreenFrameQuad() {
	static ShaderProgram* quadProg = ShaderProgram::Build()
	.WithVertexShader(
		Resources::Get<VertexShader>("./res/shaders/fullscreen.vert")
	)
	.WithPixelShader(
		Resources::Get<PixelShader>("./res/shaders/blit.frag")
	).Link();

	static Mesh* quadMesh = Resources::Get<Mesh>("./res/models/fullscreenquad.obj", VertexSpec::Mesh);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(quadMesh->GetHandle());

	glUseProgram(quadProg->GetHandle());

	glBindTexture(GL_TEXTURE_2D, this->colorPassOutputTexture);
	
	glDrawElements(GL_TRIANGLES, quadMesh->GetTriangleCount() * 3, GL_UNSIGNED_INT, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glEnable(GL_DEPTH_TEST);
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
