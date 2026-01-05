#include <Graphics.h>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <MeshRenderer.h>
#include <Camera.h>
#include <Skybox.h>
#include <Resources.h>
#include <Light.h>
#include <Texture.h>
#include <LightSystem.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

#include <GLFW/glfw3.h>

#define LIGHT_GRID_SIZE 16

static Texture2D* testTexture;

SceneGraphics::SceneGraphics(Scene* scene):
scene(scene),
currentRenders(),
globalUniformsBuffer(0),
objectUniformsBuffer(0),
depthPrepassFramebuffer(0),
depthPrepassDepthTexture(0),
colorPassFramebuffer(0),
colorPassOutputTexture(0) {
	glGenBuffers(1, &this->globalUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderGlobalUniforms), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glGenBuffers(1, &this->objectUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->objectUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderObjectUniforms), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, objectUniformsBuffer);
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

		if (testTexture) {
			delete testTexture;
		}
		
		testTexture = new Texture2D(this->screenResolution.x, this->screenResolution.y, TextureFormat::RGFloat);
	}
}

void SceneGraphics::RenderObjects(const ShaderGlobalUniforms& globalUniforms, SceneGraphics::PassType pass) {
	ShaderObjectUniforms objectUniforms;

	for (auto node : this->currentRenders) {
		const Mesh::SubMesh* mesh = node.mesh;
		const Material* mat = node.material;

		if (!mat) {
			continue;
		}

		if (mat->GetShader()->IgnoresDepthPrepass() && pass == PassType::DepthPrepass) {
			continue;
		}

		if (!mat->GetShader()->CastsShadows() && pass == PassType::Shadows) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = node.transformation;
		objectUniforms.Object_MVPMatrix = globalUniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		mat->Bind();

		glActiveTexture(GL_TEXTURE31);
		glBindTexture(GL_TEXTURE_2D, this->scene->GetLightSystem()->shadowAtlasDepthTexture);
		glUniform1i(glGetUniformLocation(mat->GetShader()->handle, "shadowMask"), 31);
		
		glBindVertexArray(mesh->GetVertexArrayHandle());

		if (mat->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) mesh->GetType());

			// glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

			if (node.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, node.instanceCount);
			}

			// glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
		}
		else {
			if (node.instanceCount <= 0) {
				glDrawElements(mesh->GetDrawMode(), mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(mesh->GetDrawMode(), mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, node.instanceCount);
			}
		}

		glBindVertexArray(0);
	}
}

void SceneGraphics::BindGlobalUniformBuffer(const ShaderGlobalUniforms& globalUniforms) {
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(globalUniforms), &globalUniforms, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
	globalUniforms.Global_CameraWorldPos = glm::vec4(mainCamera->GlobalTransform().Position().value, 0.0);
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = mainCamera->GetFarPlane();
	globalUniforms.Global_CameraNearPlane = mainCamera->GetNearPlane();
	globalUniforms.Global_CameraFov = mainCamera->GetFovRad();

	BindGlobalUniformBuffer(globalUniforms);

	glCullFace(GL_BACK);

	glBindFramebuffer(GL_FRAMEBUFFER, this->depthPrepassFramebuffer);

	glClear(GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);

	RenderObjects(globalUniforms, PassType::DepthPrepass);

	glBindFramebuffer(GL_FRAMEBUFFER, this->colorPassFramebuffer);

	Skybox* sky = Skybox::GetCurrentSkybox();

	if (!sky) {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glDepthFunc(GL_LEQUAL);

	RenderObjects(globalUniforms, PassType::Color);

	if (sky) {
		sky->GetSKyMaterial()->Bind();
		glBindVertexArray(sky->GetSkyMesh()->SubMeshAt(0).GetVertexArrayHandle());
		glDrawElements(GL_TRIANGLES, sky->GetSkyMesh()->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
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

	static Mesh* quadMesh = Resources::Get<Mesh>("./res/models/fullscreenquad.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());

	glUseProgram(quadProg->GetHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->colorPassOutputTexture);
	
	glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glEnable(GL_DEPTH_TEST);
}

void SceneGraphics::DrawMesh(MeshRenderer* renderer) {
	DrawMeshInstanced(renderer, 0);
}

void SceneGraphics::DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation) {
	DrawMeshInstanced(mesh, subMeshIndex, material, transformation, 0);
}

void SceneGraphics::DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount) {
	// for (const Mesh::SubMesh& mesh : renderer->GetMesh()->GetSubMeshes()) {
	for (int i = 0; i < renderer->GetMesh()->GetSubMeshCount(); i++) {
		const Mesh::SubMesh* mesh = &renderer->GetMesh()->SubMeshAt(i);

		this->currentRenders.push_back(RenderNode(
			mesh,
			renderer->GetMaterial(mesh->GetMaterialIndex()),
			instanceCount,
			renderer->GlobalTransform()
		));
	}
}

void SceneGraphics::DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount) {
	this->currentRenders.push_back(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
		instanceCount,
		transformation
	));
}
