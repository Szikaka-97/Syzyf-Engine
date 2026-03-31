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

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"
#include "animation/SkeletonComponent.h"
#include "animation/SkeletonSystem.h"
#include "include/Framebuffer.h"
#include "include/Shader.h"

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

SceneGraphics::SceneGraphics(Scene* scene):
GameObjectSystem(scene),
currentRenders(),
gizmoRenders(),
globalUniformsBuffer(0),
objectUniformsBuffer(0),
mainCamera(nullptr),
mainViewport(new Viewport()) {
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

    int offsetLocation = glGetUniformLocation(mat->GetShader()->handle, "uBoneOffet");
    if (offsetLocation >= 0) {
      if (node.jointBufferOffset >= 0) {
        glUniform1i(offsetLocation, node.jointBufferOffset);
      } else {
        glUniform1i(offsetLocation, 0);
      }
    }

		if (params.pass == RenderPassType::Color) {
			int shadowmaskUniformLocation = glGetUniformLocation(mat->GetShader()->handle, "Builtin_ShadowMask");

			if (shadowmaskUniformLocation >= 0) {
				glActiveTexture(GL_TEXTURE31);
				glBindTexture(GL_TEXTURE_2D, GetLightSystem()->shadowAtlasFramebuffer->GetDepthTexture()->GetHandle());
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
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);
}

void SceneGraphics::RenderFullscreenFrameQuad() {
	static ShaderProgram* quadProg = ShaderProgram::Build()
	.WithVertexShader(
		GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
	)
	.WithPixelShader(
		GetScene()->Resources()->Get<PixelShader>("./res/shaders/blit.frag")
	).Link();

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
	.WithVertexShader(
		GetScene()->Resources()->Get<VertexShader>("./res/shaders/fullscreen.vert")
	)
	.WithPixelShader(
		GetScene()->Resources()->Get<PixelShader>("./res/shaders/transparency_composite.frag")
	).Link();

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
}

void SceneGraphics::DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount) {
  int skinningOffset = -1;
  SkeletonComponent* skeleton = renderer->GetNode()->GetObject<SkeletonComponent>();
  if (skeleton) {
    skinningOffset = skeleton->bufferOffset;
  }

	for (int i = 0; i < renderer->GetMesh()->GetSubMeshCount(); i++) {
		const Mesh::SubMesh* mesh = &renderer->GetMesh()->SubMeshAt(i);
		const Material* material = renderer->GetMaterial(mesh->GetMaterialIndex()); 

		auto& targetRenderQueue = material->GetShader()->IsTransparent() ? this->transparentRenders : this->currentRenders;

		RenderNode node(
			mesh,
			renderer->GetMaterial(mesh->GetMaterialIndex()),
			instanceCount,
			renderer->GlobalTransform(),
      renderer->GetNode()->GetLayer()
		);

    node.jointBufferOffset = skinningOffset;

    targetRenderQueue.push_back(node);
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
		RenderPassType::Color | RenderPassType::DepthPrepass | RenderPassType::Gizmos | RenderPassType::Transparent | RenderPassType::PostProcessing,
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

	RenderFullscreenFrameQuad();

	this->currentRenders.clear();
	this->transparentRenders.clear();
	this->gizmoRenders.clear();
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

	ShaderGlobalUniforms globalUniforms;
	globalUniforms.Global_ViewMatrix = camera->ViewMatrix();
	globalUniforms.Global_ProjectionMatrix = camera->ProjectionMatrix();
	globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;
	globalUniforms.Global_CameraWorldPos = glm::vec4(camera->GlobalTransform().Position().Value(), 0.0);
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = camera->GetFarPlane();
	globalUniforms.Global_CameraNearPlane = camera->GetNearPlane();
	globalUniforms.Global_CameraFov = camera->GetFovRad();

	RenderParams activeParams((RenderPassType) 0, params.viewport, false, camera->GetLayerMask());

	if ((params.pass & RenderPassType::DepthPrepass) == RenderPassType::DepthPrepass) {
		activeParams.pass = RenderPassType::DepthPrepass;

		activeParams.clearDepth = true;

		this->GetMainFramebuffer()->SetColorAttachmentEnabled(false);
		RenderScene(globalUniforms, renderTarget, activeParams);
	}

	if ((params.pass & RenderPassType::Color) == RenderPassType::Color) {
		activeParams.clearDepth = false;
		activeParams.pass = RenderPassType(RenderPassType::Color);
	
		this->GetMainFramebuffer()->SetColorAttachmentEnabled(true);
		RenderScene(globalUniforms, renderTarget, activeParams);
	}

	if ((params.pass & RenderPassType::Gizmos) == RenderPassType::Gizmos) {
		activeParams.pass = RenderPassType(RenderPassType::Gizmos);
	
		RenderScene(globalUniforms, renderTarget, activeParams);
	}

	if ((params.pass & RenderPassType::PostProcessing) == RenderPassType::PostProcessing) {
		activeParams.pass = RenderPassType(RenderPassType::PostProcessing);
	
		RenderScene(globalUniforms, renderTarget, activeParams);
	}

	if (camera == this->mainCamera && (params.pass & RenderPassType::Transparent) == RenderPassType::Transparent) {
		activeParams.pass = RenderPassType(RenderPassType::Transparent);
	
		RenderScene(globalUniforms, this->transparentPassFramebuffer, activeParams);
	}
}

void SceneGraphics::RenderScene(const ShaderGlobalUniforms& uniforms, Framebuffer* framebuffer, const RenderParams& params) {
	glBindFramebuffer(GL_FRAMEBUFFER, framebuffer->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	BindGlobalUniformBuffer(uniforms);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, objectUniformsBuffer);

  SkeletonSystem* skeletonSystem = GetScene()->GetComponent<SkeletonSystem>();
  if (skeletonSystem) {
    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skeletonSystem->GetSkinningBufferHandle());
  }

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

	if (((int) params.pass & (int) RenderPassType::Transparent) != 0) {
		glDepthMask(GL_FALSE);
		glEnable(GL_BLEND);
		glBlendFunci(0, GL_ONE, GL_ONE);
		glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
		glBlendEquation(GL_FUNC_ADD);

		glClearBufferfv(GL_COLOR, 0, &glm::zero<glm::vec4>()[0]); 
		glClearBufferfv(GL_COLOR, 1, &glm::one<glm::vec4>()[0]);

		RenderParams transparentPassParams = params;
		transparentPassParams.pass = RenderPassType::Transparent;

		RenderObjects(uniforms, transparentPassParams);
		
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glDepthMask(GL_TRUE);
		glDisable(GL_BLEND);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
					this->GetMainFramebuffer()->GetColorTexture()->GetHandle(),
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
					this->mainViewport->GetSize().x,
					this->mainViewport->GetSize().y,
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

void SceneGraphics::RenderScene(const ShaderGlobalUniforms& uniforms, Viewport* viewport, const RenderParams& params) {
	RenderScene(uniforms, viewport->GetFramebuffer(), params);
}

void SceneGraphics::RenderScene(const CameraData& camera, Viewport* viewport, const RenderParams& params) {
	RenderScene(camera, viewport->GetFramebuffer(), params);
}

void SceneGraphics::RenderScene(Camera* camera, Viewport* viewport, const RenderParams& params) {
	RenderScene(camera, viewport->GetFramebuffer(), params);
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
