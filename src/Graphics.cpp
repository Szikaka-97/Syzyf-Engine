#include <Graphics.h>

#include <glad/glad.h>
#include <spdlog/spdlog.h>
#include <glm/gtc/matrix_access.hpp>
#include <imgui.h>

#include <MeshRenderer.h>
#include <Camera.h>
#include <Skybox.h>
#include <Resources.h>
#include <Light.h>
#include <Texture.h>
#include <LightSystem.h>
#include <PostProcessingSystem.h>
#include <ReflectionProbeSystem.h>
#include <Frustum.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

#include <GLFW/glfw3.h>

#define LIGHT_GRID_SIZE 16

Frustum ComputeFrustum(const glm::mat4& projectionMatrix) {
	Frustum result;

	const glm::vec4 planeLeftParams = glm::normalize(-(glm::row(projectionMatrix, 3) + glm::row(projectionMatrix, 0)));
	const glm::vec4 planeRightParams = glm::normalize(-(glm::row(projectionMatrix, 3) - glm::row(projectionMatrix, 0)));
	const glm::vec4 planeBottomParams = glm::normalize(-(glm::row(projectionMatrix, 3) + glm::row(projectionMatrix, 1)));
	const glm::vec4 planeTopParams = glm::normalize(-(glm::row(projectionMatrix, 3) - glm::row(projectionMatrix, 1)));
	const glm::vec4 planeNearParams = glm::normalize(-(glm::row(projectionMatrix, 3) + glm::row(projectionMatrix, 2)));
	const glm::vec4 planeFarParams = glm::normalize(-(glm::row(projectionMatrix, 3) - glm::row(projectionMatrix, 2)));

	result.left = Plane(glm::vec3(planeLeftParams), planeLeftParams.w);
	result.right = Plane(glm::vec3(planeRightParams), planeRightParams.w);
	result.bottom = Plane(glm::vec3(planeBottomParams), planeBottomParams.w);
	result.top = Plane(glm::vec3(planeTopParams), planeTopParams.w);
	result.nearPlane = Plane(glm::vec3(planeNearParams), planeNearParams.w);
	result.farPlane = Plane(glm::vec3(planeFarParams), planeFarParams.w);
	
	return result;
}

bool TestPlane(const Plane& plane, const BoundingBox& bounds) {
	const glm::vec3 n = plane.normal;
	const float d = plane.distance;

	const glm::vec3 c = bounds.center;
	const glm::vec3 h = bounds.GetExtents();

	const float e = h.x * glm::abs(
		glm::dot(n, glm::vec3(bounds.axisU))
	) + h.y * glm::abs(
		glm::dot(n, glm::vec3(bounds.axisV))
	) + h.z * glm::abs(
		glm::dot(n, glm::vec3(bounds.axisW))
	);

	const float s = glm::dot(c, n) + d;

	return s - e <= 0;
}

bool TestFrustum(const Frustum& frustum, const BoundingBox& bounds) {
	return (
		TestPlane(frustum.left, bounds)
		&&
		TestPlane(frustum.right, bounds)
		&&
		TestPlane(frustum.bottom, bounds)
		&&
		TestPlane(frustum.top, bounds)
		&&
		TestPlane(frustum.farPlane, bounds)
	);
}

RenderParams::RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth):
pass(pass),
viewport(viewport),
clearDepth(clearDepth) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation):
mesh(mesh),
material(material),
instanceCount(instanceCount),
transformation(transformation),
bounds(mesh->GetBounds()) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, const BoundingBox& bounds):
mesh(mesh),
material(material),
instanceCount(instanceCount),
transformation(transformation),
bounds(bounds) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation):
mesh(mesh),
material(material),
ignoreDepth(ignoreDepth),
transformation(transformation),
bounds(mesh->GetBounds()) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, const BoundingBox& bounds):
mesh(mesh),
material(material),
ignoreDepth(ignoreDepth),
transformation(transformation),
bounds(bounds) { }

SceneGraphics::SceneGraphics(Scene* scene):
SceneComponent(scene),
currentRenders(),
gizmoRenders(),
globalUniformsBuffer(0),
objectUniformsBuffer(0),
depthPrepassFramebuffer(nullptr),
depthPrepassDepthTexture(nullptr),
colorPassFramebuffer(nullptr),
colorPassOutputTexture(nullptr),
screenResolution(0) {
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

	this->lightSystem = GetScene()->AddComponent<LightSystem>();
	this->postProcessing = GetScene()->AddComponent<PostProcessingSystem>();
	this->envMapping = GetScene()->AddComponent<ReflectionProbeSystem>();
}

glm::vec2 SceneGraphics::GetScreenResolution() const {
	return this->screenResolution;
}

void SceneGraphics::UpdateScreenResolution(glm::vec2 newResolution) {
	if (this->screenResolution != newResolution) {
		this->screenResolution = newResolution;

		if (this->depthPrepassDepthTexture) {
			delete this->depthPrepassDepthTexture;
		}

		if (this->colorPassOutputTexture) {
			delete this->colorPassOutputTexture;
		}

		this->depthPrepassDepthTexture = new Texture2D(newResolution.x, newResolution.y, Texture::DepthBuffer);

		this->colorPassOutputTexture = new Texture2D(newResolution.x, newResolution.y, Texture::HDRColorBuffer);

		if (!this->depthPrepassFramebuffer) {
			this->depthPrepassFramebuffer = new Framebuffer((Texture2D*) nullptr, 0, this->depthPrepassDepthTexture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, this->depthPrepassFramebuffer->GetHandle());

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		else {
			this->depthPrepassFramebuffer->SetDepthTexture(this->depthPrepassDepthTexture, 0);
		}

		if (!this->colorPassFramebuffer) {
			this->colorPassFramebuffer = new Framebuffer(this->colorPassOutputTexture, 0, this->depthPrepassDepthTexture, 0);

			glBindFramebuffer(GL_FRAMEBUFFER, this->colorPassFramebuffer->GetHandle());

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
		}
		else {
			this->colorPassFramebuffer->SetColorTexture(this->colorPassOutputTexture, 0);
			this->colorPassFramebuffer->SetDepthTexture(this->depthPrepassDepthTexture, 0);
		}

		if (GetPostProcessing()) {
			GetPostProcessing()->UpdateBufferResolution(newResolution);
		}
	}
}

LightSystem* SceneGraphics::GetLightSystem() {
	return this->lightSystem;
}

PostProcessingSystem* SceneGraphics::GetPostProcessing() {
	return this->postProcessing;
}

ReflectionProbeSystem* SceneGraphics::GetEnvMapping() {
	return this->envMapping;
}

void SceneGraphics::RenderObjects(const ShaderGlobalUniforms& globalUniforms, RenderParams params) {
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(globalUniforms.Global_VPMatrix);

	bool drawsGizmos = ((int) params.pass & (int) RenderPassType::Gizmos) != 0;

	std::vector<RenderNode>& renders = drawsGizmos ? this->gizmoRenders : this->currentRenders;

	for (const RenderNode& node : renders) {
		const Mesh::SubMesh* mesh = node.mesh;
		const Material* mat = node.material;

		if (!TestFrustum(viewFrustum, node.bounds.Transform(node.transformation))) {
			continue;
		}

		if (!mat) {
			spdlog::warn("Tried to render a mesh with no material!");
			continue;
		}

		if (mat->GetShader()->IgnoresDepthPrepass() && params.pass == RenderPassType::DepthPrepass) {
			continue;
		}

		if (!mat->GetShader()->CastsShadows() && params.pass == RenderPassType::Shadows) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = node.transformation;
		objectUniforms.Object_MVPMatrix = globalUniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);

		mat->Bind();

		if (params.pass == RenderPassType::Color) {
			int shadowmaskUniformLocation = glGetUniformLocation(mat->GetShader()->handle, "Builtin_ShadowMask");

			if (shadowmaskUniformLocation >= 0) {
				glActiveTexture(GL_TEXTURE31);
				glBindTexture(GL_TEXTURE_2D, GetLightSystem()->shadowAtlasDepthTexture->GetHandle());
				glUniform1i(shadowmaskUniformLocation, 31);
			}

			int irradianceMapUniformLocation = glGetUniformLocation(mat->GetShader()->handle, "Builtin_EnvIrradianceMap");
			int prefilterMapUniformLocation = glGetUniformLocation(mat->GetShader()->handle, "Builtin_EnvPrefilterMap");
			int brdfConvolutionMapUniformLocation = glGetUniformLocation(mat->GetShader()->handle, "Builtin_BRDFConvolutionMap");

			ReflectionProbe* closestProbe = nullptr;

			if (irradianceMapUniformLocation >= 0 || prefilterMapUniformLocation >= 0 || brdfConvolutionMapUniformLocation >= 0){ 
				closestProbe = envMapping->GetClosestProbe(mesh->GetBounds().Transform(node.transformation).center);
			}

			if (closestProbe) {
				if (irradianceMapUniformLocation >= 0) {
					glActiveTexture(GL_TEXTURE30);
					glBindTexture(GL_TEXTURE_CUBE_MAP, closestProbe->GetIrradianceMap()->GetHandle());
					glUniform1i(irradianceMapUniformLocation, 30);
				}
				if (prefilterMapUniformLocation >= 0) {
					glActiveTexture(GL_TEXTURE29);
					glBindTexture(GL_TEXTURE_CUBE_MAP, closestProbe->GetPrefilterMap()->GetHandle());
					glUniform1i(prefilterMapUniformLocation, 29);
				}
				if (brdfConvolutionMapUniformLocation >= 0) {
					glActiveTexture(GL_TEXTURE28);
					glBindTexture(GL_TEXTURE_2D, envMapping->BRDFConvolutionMap()->GetHandle());
					glUniform1i(brdfConvolutionMapUniformLocation, 28);
				}
			}
		}
		
		glBindVertexArray(mesh->GetVertexArrayHandle());

		if (drawsGizmos && node.ignoreDepth) {
			glDisable(GL_DEPTH_TEST);
		}

		if (mat->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) mesh->GetType());

			if (drawsGizmos || node.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, node.instanceCount);
			}
		}
		else {
			if (drawsGizmos || node.instanceCount <= 0) {
				glDrawElements(mesh->GetDrawMode(), mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(mesh->GetDrawMode(), mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, node.instanceCount);
			}
		}

		if (drawsGizmos && node.ignoreDepth) {
			glEnable(GL_DEPTH_TEST);
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
	glBindTexture(GL_TEXTURE_2D, this->colorPassOutputTexture->GetHandle());
	
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

void SceneGraphics::DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds) {
	DrawMeshInstanced(mesh, subMeshIndex, material, transformation, 0, bounds);
}

void SceneGraphics::DrawGizmoMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, bool ignoresDepth) {
	this->gizmoRenders.push_back(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
		ignoresDepth,
		transformation
	));
}

void SceneGraphics::DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount) {
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

void SceneGraphics::DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, const BoundingBox& bounds) {
	this->currentRenders.push_back(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
		instanceCount,
		transformation,
		bounds
	));
}

void SceneGraphics::Render() {
	Camera* mainCamera = Camera::GetMainCamera();

	ShaderGlobalUniforms globalUniforms;
	globalUniforms.Global_ViewMatrix = mainCamera->ViewMatrix();
	globalUniforms.Global_ProjectionMatrix = mainCamera->ProjectionMatrix();
	globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;
	globalUniforms.Global_CameraWorldPos = glm::vec4(mainCamera->GlobalTransform().Position().Value(), 0.0);
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = mainCamera->GetFarPlane();
	globalUniforms.Global_CameraNearPlane = mainCamera->GetNearPlane();
	globalUniforms.Global_CameraFov = mainCamera->GetFovRad();

	RenderParams params(RenderPassType::DepthPrepass, glm::vec4(0, 0, this->screenResolution.x, this->screenResolution.y));

	params.clearDepth = true;

	RenderScene(globalUniforms, this->depthPrepassFramebuffer, params);

	params.clearDepth = false;
	params.pass = RenderPassType(RenderPassType::Color);

	RenderScene(globalUniforms, this->colorPassFramebuffer, params);

	params.pass = RenderPassType(RenderPassType::Gizmos);

	RenderScene(globalUniforms, this->colorPassFramebuffer, params);

	params.pass = RenderPassType(RenderPassType::PostProcessing);

	RenderScene(globalUniforms, this->colorPassFramebuffer, params);

	RenderFullscreenFrameQuad();

	glBindVertexArray(0);
	glUseProgram(0);

	this->currentRenders.clear();

	this->gizmoRenders.clear();
}

void SceneGraphics::RenderScene(const ShaderGlobalUniforms& uniforms, Framebuffer* framebuffer, const RenderParams& params) {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	BindGlobalUniformBuffer(uniforms);

	if (params.clearDepth) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}

	if (((int) params.pass & (int) RenderPassType::DepthPrepass) != 0) {
		if (((int) params.pass & (int) RenderPassType::Shadows) == (int) RenderPassType::Shadows) {
			// glCullFace(GL_FRONT);
		}
		else {
		}
		glCullFace(GL_BACK);
	
		glDepthFunc(GL_LESS);
	
		RenderParams depthPrepassParams = params;

		RenderObjects(uniforms, depthPrepassParams);
	}

	if (((int) params.pass & (int) RenderPassType::Color) != 0) {
		Skybox* sky = Skybox::GetCurrentSkybox();

		if (!sky) {
			glClearColor(0, 0, 0, 0);
			glClear(GL_COLOR_BUFFER_BIT);
		}

		glCullFace(GL_BACK);
		glDepthFunc(GL_LEQUAL);

		RenderParams colorPassParams = params;
		colorPassParams.pass = RenderPassType::Color;

		RenderObjects(uniforms, colorPassParams);

		if (sky) {
			sky->GetSkyMaterial()->Bind();
			glBindVertexArray(sky->GetSkyMesh()->SubMeshAt(0).GetVertexArrayHandle());
			glDrawElements(GL_TRIANGLES, sky->GetSkyMesh()->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
		}
	}

	if (((int) params.pass & (int) RenderPassType::Gizmos) != 0) {
		RenderParams gizmoPassParams = params;
		gizmoPassParams.pass = RenderPassType::Gizmos;

		RenderObjects(uniforms, gizmoPassParams);
	}

	if (((int) params.pass & (int) RenderPassType::PostProcessing) != 0) {
		PostProcessingSystem* postProcess = GetPostProcessing();

		if (postProcess) {
			Texture2D* frameTex = dynamic_cast<Texture2D*>(framebuffer->GetColorTexture());
			Texture2D* frameDepth = dynamic_cast<Texture2D*>(framebuffer->GetDepthTexture());
			Texture2D postProcessBuffer = Texture::Wrap<Texture2D>(postProcess->GetPostProcessBuffer());
			
			PostProcessParams postProcessParams;
			postProcessParams.inputTexture = &postProcessBuffer;
			postProcessParams.outputTexture = frameTex;
			postProcessParams.depthTexture = frameDepth;
			
			for (auto* effect : *postProcess->GetAllObjects()) {
				if (!effect->IsEnabled()) {
					continue;
				}
				
				glCopyImageSubData(
					this->colorPassOutputTexture->GetHandle(),
					GL_TEXTURE_2D,
					0,
					0,
					0,
					0,
					postProcess->GetPostProcessBuffer(),
					GL_TEXTURE_2D,
					0,
					0,
					0,
					0,
					this->screenResolution.x,
					this->screenResolution.y,
					1
				);
	
				effect->OnPostProcess(&postProcessParams);
			}
		}
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderScene(const CameraData& camera, Framebuffer* framebuffer, const RenderParams& params) {
	ShaderGlobalUniforms globalUniforms;
	globalUniforms.Global_ViewMatrix = camera.ViewMatrix();
	globalUniforms.Global_ProjectionMatrix = camera.ProjectionMatrix();
	globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;
	globalUniforms.Global_CameraWorldPos = glm::vec4((glm::vec3) camera.cameraTransform[3], 0.0);
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = camera.GetFarPlane();
	globalUniforms.Global_CameraNearPlane = camera.GetNearPlane();
	globalUniforms.Global_CameraFov = camera.GetFovRad();

	RenderScene(globalUniforms, framebuffer, params);
}

void SceneGraphics::RenderScene(Camera* camera, Framebuffer* framebuffer, const RenderParams& params) {
	RenderScene(camera->GetCameraData(), framebuffer, params);
}

void SceneGraphics::OnPostRender() {
	Render();
}

void SceneGraphics::DrawImGui() {
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Graphics Debug")) {
		ImGui::Text("Resolution: %i:%i", (int) this->screenResolution.x, (int) this->screenResolution.y);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::TreePop();
	}
}

int SceneGraphics::Order() {
	return INT_MAX;
}