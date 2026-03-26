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
#include <Viewport.h>
#include <TimeSystem.h>

#include "Scene.h"
#include "include/Framebuffer.h"
#include "include/Shader.h"

#include <GLFW/glfw3.h>
#include <utility>

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

RenderParams::RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth, LayerMask layers):
pass(pass),
viewport(viewport),
clearDepth(clearDepth),
layers(layers) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, uint8_t layer):
mesh(mesh),
material(material),
instanceCount(instanceCount),
transformation(transformation),
bounds(mesh->GetBounds()),
layer(layer) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, unsigned int instanceCount, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer):
mesh(mesh),
material(material),
instanceCount(instanceCount),
transformation(transformation),
bounds(bounds),
layer(layer) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, uint8_t layer):
mesh(mesh),
material(material),
ignoreDepth(ignoreDepth),
transformation(transformation),
bounds(mesh->GetBounds()),
layer(layer) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, bool ignoreDepth, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer):
mesh(mesh),
material(material),
ignoreDepth(ignoreDepth),
transformation(transformation),
bounds(bounds),
layer(layer) { }

bool SceneGraphics::RenderNode::operator<(const SceneGraphics::RenderNode& other) const {
	if (!this->material || !other.material) {
		return false;
	}

	if (this->material->GetShader() == other.material->GetShader()) {
		return ((intptr_t) this->material) < ((intptr_t) other.material);
	}
	return ((intptr_t) this->material->GetShader()) < ((intptr_t) other.material->GetShader());
}

SceneGraphics::SceneGraphics(Scene* scene):
GameObjectSystem(scene),
currentRenders(),
gizmoRenders(),
globalUniformsBuffer(0),
objectUniformsBuffer(0),
mainCamera(nullptr),
mainViewport(new Viewport()),
currentUniforms() {
	glGenBuffers(1, &this->globalUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderGlobalUniforms), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glGenBuffers(1, &this->objectUniformsBuffer);
	glBindBuffer(GL_UNIFORM_BUFFER, this->objectUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(ShaderObjectUniforms), nullptr, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);

	this->lightSystem = GetScene()->AddComponent<LightSystem>();
	this->postProcessing = GetScene()->AddComponent<PostProcessingSystem>();
	this->envMapping = GetScene()->AddComponent<ReflectionProbeSystem>();

	this->opaquePassFramebuffer = this->mainViewport->GetFramebuffer();
	this->transparentPassFramebuffer = new Framebuffer(Framebuffer::Attachment::None, 0, 0);
	
	this->opaquePassFramebuffer->CreateColorAttachment(true, false);
	this->opaquePassFramebuffer->CreateDepthAttachment(false, false);

	this->transparentPassFramebuffer->CreateColorAttachment(true, false),
	this->transparentPassFramebuffer->CreateCustomAttachment(0, TextureParams{
		.channels = TextureChannels::Grayscale,
		.colorSpace = TextureColor::Linear,
		.format = TextureFormat::Float8
	});
	this->transparentPassFramebuffer->SetDepthTexture(this->opaquePassFramebuffer->GetDepthTexture());

	this->depthOnlyShader = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/basic.vert")
	.WithPixelShader("./res/shaders/basic.frag")
	.Link();
}

glm::vec2 SceneGraphics::GetScreenResolution() const {
	return this->mainViewport->GetSize();
}

void SceneGraphics::UpdateScreenResolution(glm::vec2 newResolution) {
	if (this->mainViewport->GetSize() != glm::uvec2(newResolution)) {

		this->mainViewport->SetSize(newResolution);
		this->transparentPassFramebuffer->SetSize(newResolution);

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

Viewport* SceneGraphics::GetMainViewport() const {
	return this->mainViewport;
}

Framebuffer* SceneGraphics::GetMainFramebuffer() const {
	return this->mainViewport->GetFramebuffer();
}

Camera* SceneGraphics::GetMainCamera() const {
	return this->mainCamera;
}

void SceneGraphics::SetMainCamera(Camera* camera) {
	this->mainCamera = camera;
}

void SceneGraphics::RenderObjects(const ShaderGlobalUniforms& globalUniforms, RenderParams params) {
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(globalUniforms.Global_VPMatrix);

	bool drawsGizmos = ((int) params.pass & (int) RenderPassType::Gizmos) != 0;
	bool drawTransparent = ((int) params.pass & (int) RenderPassType::Transparent) != 0;

	std::vector<RenderNode>& renders = drawsGizmos ? this->gizmoRenders : (drawTransparent ? this->transparentRenders : this->currentRenders);

	for (const RenderNode& node : renders) {
		if (!params.layers.Test(node.layer)) {
			continue;
		}

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
			int shadowmaskUniformLocation = glGetUniformLocation(mat->GetShader()->GetHandle(), "Builtin_ShadowMask");

			if (shadowmaskUniformLocation >= 0) {
				glActiveTexture(GL_TEXTURE31);
				glBindTexture(GL_TEXTURE_2D, GetLightSystem()->shadowAtlasFramebuffer->GetDepthTexture()->GetHandle());
				glUniform1i(shadowmaskUniformLocation, 31);
			}

			int irradianceMapUniformLocation = glGetUniformLocation(mat->GetShader()->GetHandle(), "Builtin_EnvIrradianceMap");
			int prefilterMapUniformLocation = glGetUniformLocation(mat->GetShader()->GetHandle(), "Builtin_EnvPrefilterMap");
			int brdfConvolutionMapUniformLocation = glGetUniformLocation(mat->GetShader()->GetHandle(), "Builtin_BRDFConvolutionMap");

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

void SceneGraphics::BindUniformBuffers() {
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(this->currentUniforms), &this->currentUniforms, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, objectUniformsBuffer);

}

void SceneGraphics::RenderFullscreenFrameQuad() {
	static ShaderProgram* quadProg = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/fullscreen.vert")
	.WithPixelShader("./res/shaders/blit.frag").Link();

	static Mesh* quadMesh = GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glDisable(GL_DEPTH_TEST);

	glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());

	glUseProgram(quadProg->GetHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->GetMainFramebuffer()->GetColorTexture()->GetHandle());
	
	glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glEnable(GL_DEPTH_TEST);

	glBindVertexArray(0);
	glUseProgram(0);
}

void SceneGraphics::CompositeTransparentPass() {
	static ShaderProgram* quadProg = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/fullscreen.vert")
	.WithPixelShader("./res/shaders/transparency_composite.frag").Link();

	static Mesh* quadMesh = GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, this->opaquePassFramebuffer->GetHandle());

	glDepthFunc(GL_ALWAYS);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());

	glUseProgram(quadProg->GetHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, transparentPassFramebuffer->GetColorTexture()->GetHandle());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, transparentPassFramebuffer->GetCustomAttachmentTexture(0)->GetHandle());
	
	glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
	
	glBindTexture(GL_TEXTURE_2D, 0);

	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glBindVertexArray(0);
	glUseProgram(0);
}

void SceneGraphics::DrawMesh(MeshRenderer* renderer) {
	DrawMeshInstanced(renderer, 0);
}

void SceneGraphics::DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, uint8_t layer) {
	DrawMeshInstanced(mesh, subMeshIndex, material, transformation, 0, layer);
}

void SceneGraphics::DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer) {
	DrawMeshInstanced(mesh, subMeshIndex, material, transformation, 0, bounds, layer);
}

void SceneGraphics::DrawGizmoMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, bool ignoresDepth) {
	this->gizmoRenders.push_back(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
		ignoresDepth,
		transformation,
		Layer::Gizmos
	));

	std::sort(this->gizmoRenders.begin(), this->gizmoRenders.end());
}

void SceneGraphics::DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount) {
	for (int i = 0; i < renderer->GetMesh()->GetSubMeshCount(); i++) {
		const Mesh::SubMesh* mesh = &renderer->GetMesh()->SubMeshAt(i);

		const Material* material = renderer->GetMaterial(mesh->GetMaterialIndex()); 

		auto& targetRenderQueue = material->GetShader()->IsTransparent() ? this->transparentRenders : this->currentRenders;

		targetRenderQueue.push_back(RenderNode(
			mesh,
			renderer->GetMaterial(mesh->GetMaterialIndex()),
			instanceCount,
			renderer->GlobalTransform(),
			renderer->GetNode()->GetLayer()
		));

		std::sort(targetRenderQueue.begin(), targetRenderQueue.end());
	}
}

void SceneGraphics::DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, uint8_t layer) {
	auto& targetRenderQueue = material->GetShader()->IsTransparent() ? this->transparentRenders : this->currentRenders;

	targetRenderQueue.push_back(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
		instanceCount,
		transformation,
		layer
	));

	std::sort(targetRenderQueue.begin(), targetRenderQueue.end());
}

void SceneGraphics::DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, const BoundingBox& bounds, uint8_t layer) {
	auto& targetRenderQueue = material->GetShader()->IsTransparent() ? this->transparentRenders : this->currentRenders;

	targetRenderQueue.push_back(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
		instanceCount,
		transformation,
		bounds,
		layer
	));

	std::sort(targetRenderQueue.begin(), targetRenderQueue.end());
}

void SceneGraphics::Render() {
	std::sort(GetAllObjects()->begin(), GetAllObjects()->end(), [](auto a, auto b) -> bool {
		return a->GetPriority() > b->GetPriority();
	});

	for (Camera* camera : *this->GetAllObjects()) {
		if (camera == this->mainCamera) {
			camera->SetAspectRatio((float) this->mainViewport->GetSize().x / this->mainViewport->GetSize().y);
			
			continue;
		}

		RenderCamera(camera);
	}

	RenderCamera(this->mainCamera, this->mainViewport, RenderParams {
		RenderPassType::Color | RenderPassType::DepthPrepass | RenderPassType::Gizmos | RenderPassType::Transparent,
		glm::vec4(
			0,
			0,
			this->mainViewport->GetSize()
		),
		false
	});

	this->mainViewport->GetFramebuffer()->Apply();

	glViewport(0, 0, this->mainViewport->GetSize().x, this->mainViewport->GetSize().y);

	CompositeTransparentPass();

	RenderCamera(this->mainCamera, this->mainViewport, RenderParams {
		RenderPassType::PostProcessing,
		glm::vec4(
			0,
			0,
			this->mainViewport->GetSize()
		),
		false
	});

	RenderFullscreenFrameQuad();

	this->currentRenders.clear();
	this->transparentRenders.clear();
	this->gizmoRenders.clear();
}

void SceneGraphics::RenderPrepass(const RenderParams& params, Framebuffer* target) {
	RenderPrepass(this->currentUniforms, params, target);
}
void SceneGraphics::RenderPrepass(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(false);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
	glClear(GL_DEPTH_BUFFER_BIT);
			
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	bool genericShaderBound = true;
	glUseProgram(this->depthOnlyShader->GetHandle());

	for (auto& render : this->currentRenders) {
		if (!render.material || !render.mesh) {
			continue;
		}

		if (!params.layers.Test(render.layer)) {
			continue;
		}

		if (render.material->GetShader()->IgnoresDepthPrepass()) {
			continue;
		}

		if (!TestFrustum(viewFrustum, render.bounds.Transform(render.transformation))) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = render.transformation;
		objectUniforms.Object_MVPMatrix = uniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		if (render.material->GetShader()->HasPragma("complex_vertex_shader")) {
			render.material->Bind();

			genericShaderBound = false;
		}
		else if (!genericShaderBound) {
			genericShaderBound = true;
			glUseProgram(this->depthOnlyShader->GetHandle());
		}

		glBindVertexArray(render.mesh->GetVertexArrayHandle());

		if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
		else {
			if (render.instanceCount <= 0) {
				glDrawElements(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderShadows(const RenderParams& params, Framebuffer* target) {
	RenderShadows(this->currentUniforms, params, target);
}
void SceneGraphics::RenderShadows(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);

	if (params.clearDepth) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}
			
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	bool genericShaderBound = true;
	glUseProgram(this->depthOnlyShader->GetHandle());

	for (auto& render : this->currentRenders) {
		if (!render.material || !render.mesh) {
			continue;
		}

		if (!params.layers.Test(render.layer)) {
			continue;
		}

		if (!render.material->GetShader()->CastsShadows()) {
			continue;
		}

		if (!TestFrustum(viewFrustum, render.bounds.Transform(render.transformation))) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = render.transformation;
		objectUniforms.Object_MVPMatrix = uniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		if (render.material->GetShader()->HasPragma("complex_vertex_shader")) {
			render.material->Bind();

			genericShaderBound = false;
		}
		else if (!genericShaderBound) {
			genericShaderBound = true;
			glUseProgram(this->depthOnlyShader->GetHandle());
		}

		glBindVertexArray(render.mesh->GetVertexArrayHandle());

		if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
		else {
			if (render.instanceCount <= 0) {
				glDrawElements(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderOpaque(const RenderParams& params, Framebuffer* target) {
	RenderOpaque(this->currentUniforms, params, target);
}
void SceneGraphics::RenderOpaque(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	Skybox* sky = Skybox::GetCurrentSkybox();

	if (!sky) {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);

	if (params.clearDepth) {
		glClear(GL_DEPTH_BUFFER_BIT);
	}
			
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	const ShaderProgram* currentProg = nullptr;

	for (auto& render : this->currentRenders) {
		if (!render.material || !render.mesh) {
			continue;
		}

		if (!params.layers.Test(render.layer)) {
			continue;
		}

		if (!TestFrustum(viewFrustum, render.bounds.Transform(render.transformation))) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = render.transformation;
		objectUniforms.Object_MVPMatrix = uniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		if (render.material->GetShader() != currentProg) {
			currentProg = render.material->GetShader();

			glUseProgram(currentProg->GetHandle());
		}

		render.material->Bind();

		int shadowmaskUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_ShadowMask");

		if (shadowmaskUniformLocation >= 0) {
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, GetLightSystem()->shadowAtlasFramebuffer->GetDepthTexture()->GetHandle());
			glUniform1i(shadowmaskUniformLocation, 31);
		}

		int irradianceMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_EnvIrradianceMap");
		int prefilterMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_EnvPrefilterMap");
		int brdfConvolutionMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_BRDFConvolutionMap");

		ReflectionProbe* closestProbe = nullptr;

		if (irradianceMapUniformLocation >= 0 || prefilterMapUniformLocation >= 0 || brdfConvolutionMapUniformLocation >= 0){ 
			closestProbe = envMapping->GetClosestProbe(render.mesh->GetBounds().Transform(render.transformation).center);
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

		glBindVertexArray(render.mesh->GetVertexArrayHandle());

		if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
		else {
			if (render.instanceCount <= 0) {
				glDrawElements(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
	}

	if (sky) {
		sky->GetSkyMaterial()->Bind();
		glBindVertexArray(sky->GetSkyMesh()->SubMeshAt(0).GetVertexArrayHandle());
		glDrawElements(GL_TRIANGLES, sky->GetSkyMesh()->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderOITransparent(const RenderParams& params, Framebuffer* target) {
	RenderOITransparent(this->currentUniforms, params, target);
}
void SceneGraphics::RenderOITransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);
	
	glCullFace(GL_BACK);
	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendFunci(0, GL_ONE, GL_ONE);
	glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glBlendEquation(GL_FUNC_ADD);

	glClearBufferfv(GL_COLOR, 0, &glm::zero<glm::vec4>()[0]); 
	glClearBufferfv(GL_COLOR, 1, &glm::one<glm::vec4>()[0]);

	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	const ShaderProgram* currentProg = nullptr;

	for (auto& render : this->transparentRenders) {
		if (!render.material || !render.mesh) {
			continue;
		}

		if (!params.layers.Test(render.layer)) {
			continue;
		}

		if (!TestFrustum(viewFrustum, render.bounds.Transform(render.transformation))) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = render.transformation;
		objectUniforms.Object_MVPMatrix = uniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		if (render.material->GetShader() != currentProg) {
			currentProg = render.material->GetShader();

			glUseProgram(currentProg->GetHandle());
		}

		render.material->Bind();

		int shadowmaskUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_ShadowMask");

		if (shadowmaskUniformLocation >= 0) {
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, GetLightSystem()->shadowAtlasFramebuffer->GetDepthTexture()->GetHandle());
			glUniform1i(shadowmaskUniformLocation, 31);
		}

		int irradianceMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_EnvIrradianceMap");
		int prefilterMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_EnvPrefilterMap");
		int brdfConvolutionMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_BRDFConvolutionMap");

		ReflectionProbe* closestProbe = nullptr;

		if (irradianceMapUniformLocation >= 0 || prefilterMapUniformLocation >= 0 || brdfConvolutionMapUniformLocation >= 0){ 
			closestProbe = envMapping->GetClosestProbe(render.mesh->GetBounds().Transform(render.transformation).center);
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

		glBindVertexArray(render.mesh->GetVertexArrayHandle());

		if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
		else {
			if (render.instanceCount <= 0) {
				glDrawElements(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
	}

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderGizmos(const RenderParams& params, Framebuffer* target) {
	RenderGizmos(this->currentUniforms, params, target);
}
void SceneGraphics::RenderGizmos(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	Skybox* sky = Skybox::GetCurrentSkybox();

	if (!sky) {
		glClearColor(0, 0, 0, 0);
		glClear(GL_COLOR_BUFFER_BIT);
	}

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LEQUAL);
			
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	const ShaderProgram* currentProg = nullptr;

	for (auto& render : this->gizmoRenders) {
		if (!render.material || !render.mesh) {
			continue;
		}

		if (!params.layers.Test(render.layer)) {
			continue;
		}

		if (!TestFrustum(viewFrustum, render.bounds.Transform(render.transformation))) {
			continue;
		}

		objectUniforms.Object_ModelMatrix = render.transformation;
		objectUniforms.Object_MVPMatrix = uniforms.Global_VPMatrix * objectUniforms.Object_ModelMatrix;
		objectUniforms.Object_NormalModelMatrix = glm::transpose(glm::inverse(glm::mat3(objectUniforms.Object_ModelMatrix)));

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
		if (render.material->GetShader() != currentProg) {
			currentProg = render.material->GetShader();

			glUseProgram(currentProg->GetHandle());
		}

		if (render.ignoreDepth) {
			glDisable(GL_DEPTH_TEST);
		}

		render.material->Bind();

		glBindVertexArray(render.mesh->GetVertexArrayHandle());

		if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
		}
		else {
			glDrawElements(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
		}

		if (render.ignoreDepth) {
			glEnable(GL_DEPTH_TEST);
		}
	}

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderPostprocess() {
	PostProcessingSystem* postProcess = GetPostProcessing();

	if (!postProcess) {
		return;
	}

	Framebuffer* ping = GetMainFramebuffer();
	Framebuffer* pong = postProcess->GetPostProcessBuffer();
	
	Texture2D* frameDepth = dynamic_cast<Texture2D*>(GetMainFramebuffer()->GetDepthTexture());

	PostProcessParams postProcessParams;
	postProcessParams.inputTexture = dynamic_cast<Texture2D*>(ping->GetColorTexture());
	postProcessParams.outputTexture = dynamic_cast<Texture2D*>(pong->GetColorTexture());
	postProcessParams.depthTexture = frameDepth;

	glCopyImageSubData(
		postProcessParams.inputTexture->GetHandle(),
		GL_TEXTURE_2D,
		0,
		0,
		0,
		0,
		postProcessParams.outputTexture->GetHandle(),
		GL_TEXTURE_2D,
		0,
		0,
		0,
		0,
		this->mainViewport->GetSize().x,
		this->mainViewport->GetSize().y,
		1
	);

	for (auto* effect : *postProcess->GetAllObjects()) {
		effect->OnPostProcess(&postProcessParams);

		std::swap(ping, pong);

		postProcessParams.inputTexture = dynamic_cast<Texture2D*>(ping->GetColorTexture());
		postProcessParams.outputTexture = dynamic_cast<Texture2D*>(pong->GetColorTexture());
	}

	this->opaquePassFramebuffer = ping;
	postProcess->SetPostProcessBuffer(pong);
}

void SceneGraphics::RenderCamera(Camera* camera, Viewport* renderTarget) {
	assert(camera != nullptr);

	Viewport* target = renderTarget;

	if (target == nullptr) {
		target = camera->GetRenderTarget();
	}

	auto defaultParams = RenderParams(
		RenderPassType::Color | RenderPassType::DepthPrepass,
		glm::vec4(
			0,
			0,
			target != nullptr ? target->GetSize() : this->mainViewport->GetSize()
		),
		false
	);

	RenderCamera(camera, target, defaultParams);
}

void SceneGraphics::RenderCamera(Camera* camera, const RenderParams& params) {
	assert(camera != nullptr);

	RenderCamera(camera, nullptr, params);
}

void SceneGraphics::RenderCamera(Camera* camera, Viewport* renderTarget, const RenderParams& params) {
	assert(camera != nullptr);

	if (renderTarget == nullptr) {
		renderTarget = camera->GetRenderTarget();
	}

	if (renderTarget == nullptr) {
		return;
	}

	this->currentUniforms.Global_ViewMatrix = camera->ViewMatrix();
	this->currentUniforms.Global_ProjectionMatrix = camera->ProjectionMatrix();
	this->currentUniforms.Global_VPMatrix = this->currentUniforms.Global_ProjectionMatrix * this->currentUniforms.Global_ViewMatrix;
	this->currentUniforms.Global_CameraWorldPos = glm::vec4(camera->GlobalTransform().Position().Value(), 0.0);
	this->currentUniforms.Global_Time = Time::Current();
	this->currentUniforms.Global_CameraFarPlane = camera->GetFarPlane();
	this->currentUniforms.Global_CameraNearPlane = camera->GetNearPlane();
	this->currentUniforms.Global_CameraFov = camera->GetFovRad();

	RenderParams activeParams((RenderPassType) 0, params.viewport, false, camera->GetLayerMask());

	BindUniformBuffers();

	if ((params.pass & RenderPassType::DepthPrepass) == RenderPassType::DepthPrepass) {
		activeParams.clearDepth = true;
		activeParams.pass = RenderPassType::DepthPrepass;

		RenderPrepass(activeParams, renderTarget->GetFramebuffer());
	}

	if ((params.pass & RenderPassType::Color) == RenderPassType::Color) {
		activeParams.clearDepth = false;
		activeParams.pass = RenderPassType(RenderPassType::Color);
	
		RenderOpaque(activeParams, renderTarget->GetFramebuffer());
	}

	if ((params.pass & RenderPassType::Gizmos) == RenderPassType::Gizmos) {
		activeParams.pass = RenderPassType(RenderPassType::Gizmos);
	
		RenderGizmos(activeParams, renderTarget->GetFramebuffer());
	}

	if (camera == this->mainCamera && (params.pass & RenderPassType::Transparent) == RenderPassType::Transparent) {
		activeParams.pass = RenderPassType(RenderPassType::Transparent);
	
		RenderOITransparent(activeParams, this->transparentPassFramebuffer);
	}

	if ((params.pass & RenderPassType::PostProcessing) == RenderPassType::PostProcessing) {
		RenderPostprocess();
	}

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::OnPostRender() {
	Render();
}

void SceneGraphics::DrawImGui() {
	ImGui::SetNextItemOpen(true, ImGuiCond_FirstUseEver);
	if (ImGui::TreeNode("Graphics Debug")) {
		ImGui::Text("Resolution: %i:%i", (int) this->mainViewport->GetSize().x, (int) this->mainViewport->GetSize().y);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / ImGui::GetIO().Framerate, ImGui::GetIO().Framerate);

		ImGui::TreePop();
	}
}

int SceneGraphics::Order() {
	return INT_MAX;
}
