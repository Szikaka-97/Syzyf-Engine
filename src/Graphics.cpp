#include <Graphics.h>

#include <glad/glad.h>
#include <spdlog/spdlog.h>

#include <MeshRenderer.h>
#include <Camera.h>
#include <Skybox.h>
#include <Resources.h>
#include <Light.h>
#include <Texture.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

#include <GLFW/glfw3.h>

#define LIGHT_GRID_SIZE 16
#define MAX_NUM_LIGHTS 128

static Texture2D* testTexture;

SceneGraphics::SceneGraphics(Scene* scene):
scene(scene),
currentRenders(),
globalUniformsBuffer(0),
depthPrepassFramebuffer(0),
depthPrepassDepthTexture(0),
colorPassFramebuffer(0),
colorPassOutputTexture(0),
lightsBuffer(0),
opaqueLightIndexList(0),
transparentLightIndexList(0),
lightIndexCounter(0),
opaqueLightsGrid(0),
transparentLightsGrid(0),
shouldRecalculateFrustums(false) {
	glGenBuffers(1, &this->globalUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderGlobalUniforms), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glGenBuffers(1, &this->lightsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->lightsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 32 + sizeof(ShaderLightRep) * MAX_NUM_LIGHTS, nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &this->opaqueLightIndexList);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->opaqueLightIndexList);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (1920*1080) / (LIGHT_GRID_SIZE*LIGHT_GRID_SIZE) * sizeof(unsigned int), nullptr, GL_STATIC_READ);

	glGenBuffers(1, &this->transparentLightIndexList);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->transparentLightIndexList);
	glBufferData(GL_SHADER_STORAGE_BUFFER, (1920*1080) / (LIGHT_GRID_SIZE*LIGHT_GRID_SIZE) * sizeof(unsigned int), nullptr, GL_STATIC_READ);

	unsigned int countersData[2] {0, 0};
	glGenBuffers(1, &this->lightIndexCounter);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->lightIndexCounter);
	glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(countersData), countersData, GL_STATIC_READ);
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	this->gridFrustumComputationShader = new ComputeShaderDispatch(Resources::Get<ComputeShader>("./res/shaders/forwardplus/compute_frustums.comp"));
	this->lightCullingShader = new ComputeShaderDispatch(Resources::Get<ComputeShader>("./res/shaders/forwardplus/light_culling.comp"));
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

		if (!this->gridFrustumsBuffer) {
			glGenBuffers(1, &this->gridFrustumsBuffer);

			this->gridFrustumComputationShader->GetData()->BindStorageBuffer(0, this->gridFrustumsBuffer);
			this->lightCullingShader->GetData()->BindStorageBuffer(0, this->gridFrustumsBuffer);
		}

		this->shouldRecalculateFrustums = true;

		if (this->opaqueLightsGrid) {
			delete this->opaqueLightsGrid;
		}
		if (this->transparentLightsGrid) {
			delete this->transparentLightsGrid;
		}
		
		this->opaqueLightsGrid = new Texture2D(std::ceil((float) newResolution.x / LIGHT_GRID_SIZE), std::ceil((float) newResolution.y / LIGHT_GRID_SIZE), TextureFormat::RGUInt);
		this->transparentLightsGrid = new Texture2D(std::ceil((float) newResolution.x / LIGHT_GRID_SIZE), std::ceil((float) newResolution.y / LIGHT_GRID_SIZE), TextureFormat::RGUInt);
		
		testTexture = new Texture2D(this->screenResolution.x, this->screenResolution.y, TextureFormat::RGFloat);
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
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBufferBase(GL_UNIFORM_BUFFER, 1, node.renderer->GetUniformBufferHandle());

		glBindBuffer(GL_UNIFORM_BUFFER, node.renderer->GetUniformBufferHandle());
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
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
	globalUniforms.Global_CameraWorldPos = glm::vec4(mainCamera->GlobalTransform().Position().value, 0.0);
	globalUniforms.Global_Time = (float) glfwGetTime();

	struct {
		glm::mat4 inverseProjection;
		glm::vec2 screenSize;
	} screenToViewParams;

	screenToViewParams.inverseProjection = glm::inverse(globalUniforms.Global_ProjectionMatrix);
	screenToViewParams.screenSize = this->screenResolution;

	if (shouldRecalculateFrustums) {
		const int FrustumSize = 64;
		const int FrustumGridSize = 16;

		shouldRecalculateFrustums = false;

		int gridSizeX = std::ceil(this->screenResolution.x / FrustumGridSize);
		int gridSizeY = std::ceil(this->screenResolution.y / FrustumGridSize);

		glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->gridFrustumsBuffer);
		glBufferData(GL_SHADER_STORAGE_BUFFER, FrustumSize * gridSizeX * gridSizeY, nullptr, GL_STATIC_DRAW);
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

		this->gridFrustumComputationShader->GetData()->SetUniformBuffer(0, &screenToViewParams);
		this->gridFrustumComputationShader->Dispatch(
			std::ceil((float) gridSizeX / FrustumGridSize),
			std::ceil((float) gridSizeY / FrustumGridSize),
			1
		);
	}

	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(globalUniforms), &globalUniforms, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->lightsBuffer);
	
	glm::vec4 ambientLight{1.0, 1.0, 1.0, 0.05};
	auto lightList = this->scene->GetSceneLights();

	int lightIndex = 0;
	for (const auto& l : lightList) {
		if (lightIndex >= MAX_NUM_LIGHTS) {
			break;
		}

		if (l->IsDirty()) {
			ShaderLightRep rep = l->GetShaderRepresentation();
	
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 32 + sizeof(ShaderLightRep) * lightIndex, sizeof(rep), &rep);
		}

		lightIndex++;
	}

	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ambientLight), &ambientLight);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 16, sizeof(lightIndex), &lightIndex);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, this->depthPrepassFramebuffer);

	glClear(GL_DEPTH_BUFFER_BIT);

	glDepthFunc(GL_LESS);

	RenderObjects(globalUniforms);

	Texture2D depthTexture = Texture::Wrap<Texture2D>(this->depthPrepassDepthTexture);

	this->lightCullingShader->GetData()->BindStorageBuffer(0, this->gridFrustumsBuffer);
	this->lightCullingShader->GetData()->BindStorageBuffer(1, this->lightsBuffer);
	this->lightCullingShader->GetData()->BindStorageBuffer(2, this->opaqueLightIndexList);
	this->lightCullingShader->GetData()->BindStorageBuffer(3, this->transparentLightIndexList);
	this->lightCullingShader->GetData()->BindStorageBuffer(4, this->lightIndexCounter);
	
	this->lightCullingShader->GetData()->SetValue("depthTexture", &depthTexture);
	this->lightCullingShader->GetData()->SetValue("Light_OpaqueLightGrid", this->opaqueLightsGrid);
	this->lightCullingShader->GetData()->SetValue("Light_TransparentLightGrid", this->transparentLightsGrid);
	this->lightCullingShader->GetData()->SetValue("testTexture", testTexture);

	this->lightCullingShader->GetData()->SetUniformBuffer(0, &globalUniforms);
	this->lightCullingShader->GetData()->SetUniformBuffer(3, &screenToViewParams);

	this->lightCullingShader->Dispatch(
		std::ceil(this->screenResolution.x / 16),
		std::ceil(this->screenResolution.y / 16),
		1
	);

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

	glActiveTexture(GL_TEXTURE0);
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
