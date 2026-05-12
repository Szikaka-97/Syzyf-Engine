#include <Graphics.h>

#include <cmath>
#include <glad/glad.h>
#include <glm/geometric.hpp>
#include <random>
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
#include "animation/SkeletonSystem.h"
#include "animation/SkeletonComponent.h"

#include "Scene.h"
#include "include/Framebuffer.h"
#include "include/Shader.h"

#include <utility>

#define LIGHT_GRID_SIZE 16

RenderParams::RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth, LayerMask layers):
pass(pass),
viewport(viewport),
clearDepth(clearDepth),
layers(layers) { }

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer, unsigned int instanceCount, GLuint instanceSSBO, bool ignoreDepth) :
mesh(mesh),
material(material),
transformation(transformation),
bounds(bounds),
layer(layer),
indirectBuffer(0),
indirectBufferOffset(0),
instanceSSBO(instanceSSBO),
isIndirect(false) {
    if (ignoreDepth) {
        this->ignoreDepth = true;
    } else {
        this->instanceCount = instanceCount;
    }
}

SceneGraphics::RenderNode::RenderNode(const Mesh::SubMesh* mesh, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer, GLuint indirectBuffer, GLuint indirectBufferOffset, GLuint instanceSSBO) :
mesh(mesh),
material(material),
instanceCount(0),
transformation(transformation),
bounds(bounds),
layer(layer),
indirectBuffer(indirectBuffer),
indirectBufferOffset(indirectBufferOffset),
instanceSSBO(instanceSSBO),
isIndirect(true) { }

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
opaqueRenders(),
gizmoRenders(),
transparentRenders(),
oitTransparentRenders(),
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

    // SSAO Framebuffer
    TextureParams ssaoParams = {
        .channels = TextureChannels::Grayscale,
        .colorSpace = TextureColor::Linear,
        .format = TextureFormat::Ubyte
    };
    this->ssaoFramebuffer = new Framebuffer(Framebuffer::Attachment::None, 0, 0);
    this->ssaoFramebuffer->CreateCustomAttachment(0, ssaoParams);
    this->ssaoBlurFramebuffer = new Framebuffer(Framebuffer::Attachment::None, 0, 0);
    this->ssaoBlurFramebuffer->CreateCustomAttachment(0, ssaoParams);
	
	this->opaquePassFramebuffer->CreateColorAttachment(true, false);
	this->opaquePassFramebuffer->CreateDepthAttachment(false, false);

	this->transparentPassFramebuffer->CreateColorAttachment(true, false),
	this->transparentPassFramebuffer->CreateCustomAttachment(0, TextureParams{
		.channels = TextureChannels::Grayscale,
		.colorSpace = TextureColor::Linear,
		.format = TextureFormat::Float8
	});
	this->transparentPassFramebuffer->SetDepthTexture(this->opaquePassFramebuffer->GetDepthTexture());

	this->volumetricPassFramebuffer = new Framebuffer(Framebuffer::Attachment::None, 0, 0);
	this->volumetricPassFramebuffer->CreateColorAttachment(true, false);

    this->uiQuadMesh = this->GetScene()->Resources()->Get<Mesh>("./res/models/uiQuad.obj");

    this->SetupShaders();
    this->GenerateSSAOKernelAndTexture();

    TextureParams normalBufferParams = TextureParams {
        .channels = TextureChannels::RGB,
        .colorSpace = TextureColor::Linear,
        .format = TextureFormat::Float
    };
    this->opaquePassFramebuffer->CreateCustomAttachment(0, normalBufferParams);
}

glm::vec2 SceneGraphics::GetScreenResolution() const {
	return this->mainViewport->GetSize();
}

void SceneGraphics::UpdateScreenResolution(glm::vec2 newResolution) {
	if (this->mainViewport->GetSize() != glm::uvec2(newResolution)) {

		this->mainViewport->SetSize(newResolution);
		this->transparentPassFramebuffer->SetSize(newResolution);
		this->volumetricPassFramebuffer->SetSize(newResolution * this->volumetricPassResolutionScale);
        
        // SSAO
        glm::vec2 ssaoResolution = glm::ceil(newResolution * this->ssaoSettings.resolutionScale);
        this->ssaoFramebuffer->SetSize(ssaoResolution);
        this->ssaoBlurFramebuffer->SetSize(ssaoResolution);

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

void SceneGraphics::BindUniformBuffers() {
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBuffer(GL_UNIFORM_BUFFER, this->globalUniformsBuffer);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(this->currentUniforms), &this->currentUniforms, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_UNIFORM_BUFFER, 0);
	
	glBindBufferBase(GL_UNIFORM_BUFFER, 0, this->globalUniformsBuffer);

	glBindBufferBase(GL_UNIFORM_BUFFER, 1, objectUniformsBuffer);

    if (auto* skeletonSystem = GetScene()->GetComponent<SkeletonSystem>()) {
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, skeletonSystem->GetSkinningBufferHandle());
    }
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

void SceneGraphics::CompositeVolumetricPass() {
	static ShaderProgram *quadProgVolumetric = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/fullscreen.vert")
	.WithPixelShader("./res/shaders/fog/fog_volume_blit.frag")
	.Link();

	static Mesh *quadMesh = GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj");

	glBindFramebuffer(GL_FRAMEBUFFER, this->opaquePassFramebuffer->GetHandle());

	glViewport(0, 0, this->opaquePassFramebuffer->GetSize().x, this->opaquePassFramebuffer->GetSize().y);

	glDepthMask(false);
	glEnable(GL_BLEND);
	glCullFace(GL_BACK);

	glBlendFunc(GL_ONE, GL_SRC_ALPHA);

	glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());
	glUseProgram(quadProgVolumetric->GetHandle());

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->volumetricPassFramebuffer->GetColorTexture()->GetHandle());

	glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);

	glBindTexture(GL_TEXTURE_2D, 0);
	glBindVertexArray(0);
	glUseProgram(0);

	glDepthMask(GL_TRUE);
	glDisable(GL_BLEND);
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
	EnqueueGizmo(RenderNode(
		&mesh->SubMeshAt(subMeshIndex),
		material,
        transformation,
        mesh->SubMeshAt(subMeshIndex).GetBounds(),
		Layer::Gizmos,
        0,
        0,
        ignoresDepth
	));
}

void SceneGraphics::DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount) {
    if (!renderer || !renderer->GetMesh()) return;

    int skinningOffset = -1;
    if (auto* skeleton = renderer->GetNode()->GetObject<SkeletonComponent>()) {
        skinningOffset = skeleton->bufferOffset;
    }

    for (int i = 0; i < renderer->GetMesh()->GetSubMeshCount(); i++) {
        const Mesh::SubMesh* mesh = &renderer->GetMesh()->SubMeshAt(i);
        const Material* material = renderer->GetMaterial(mesh->GetMaterialIndex());

        BoundingBox bounds = mesh->GetBounds();
        if (skinningOffset >= 0) {
            bounds = BoundingBox(glm::vec3(-100000.0f), glm::vec3(100000.0f));
        }

        RenderNode node = RenderNode(mesh, material, renderer->GlobalTransform().Value(), bounds, renderer->GetNode()->GetLayer());
        node.jointBufferOffset = skinningOffset;
		
        if (material->GetShader()->HasPragma("transparent")) {
            EnqueueOrderedTransparent(node);
        } else if (material->GetShader()->HasPragma("oit_transparent")) {
            EnqueueOITransparent(node);
        } else if (material->GetShader()->HasPragma("additive")) {
            EnqueueAdditive(node);
        } else if (material->GetShader()->HasPragma("volumetric")) {
            EnqueueVolumetric(node);
        } else {
            EnqueueOpaque(node);
        }
    }
}

void SceneGraphics::DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, GLuint instanceSSBO, uint8_t layer) {
    DrawMeshInstanced(
        mesh, subMeshIndex, material, transformation, instanceCount, 
        mesh->SubMeshAt(subMeshIndex).GetBounds(), instanceSSBO, layer
    );
}

void SceneGraphics::DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, const BoundingBox& bounds, GLuint instanceSSBO, uint8_t layer) {
    
    RenderNode node(&mesh->SubMeshAt(subMeshIndex), material, transformation, bounds, layer, instanceCount, instanceSSBO, false);

    if (material->GetShader()->HasPragma("transparent")) {
        EnqueueOrderedTransparent(node);
        return;
    } else if (material->GetShader()->HasPragma("oit_transparent")) {
        EnqueueOITransparent(node);
        return;
    } else if (material->GetShader()->HasPragma("additive")) {
        EnqueueAdditive(node);
        return;
    } else if (material->GetShader()->HasPragma("volumetric")) {
        EnqueueVolumetric(node);
        return;
    }

    EnqueueOpaque(node);
}

void SceneGraphics::DrawMeshIndirect(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, GLuint indirectBuffer, GLuint indirectBufferOffset, GLuint instanceSSBO, const BoundingBox& bounds, uint8_t layer) {
    RenderNode node = RenderNode(&mesh->SubMeshAt(subMeshIndex), material, transformation, bounds, layer, indirectBuffer, indirectBufferOffset, instanceSSBO);

   	if (material->GetShader()->HasPragma("transparent")) {
		EnqueueOrderedTransparent(node);

		return;
	} else if (material->GetShader()->HasPragma("oit_transparent")) {
		EnqueueOITransparent(node);

		return;
	} else if (material->GetShader()->HasPragma("additive")) {
		EnqueueAdditive(node);

		return;
	} else if (material->GetShader()->HasPragma("volumetric")) {
		EnqueueVolumetric(node);

		return;
	}

	EnqueueOpaque(node); 
}

void SceneGraphics::DrawUi(const glm::vec4& finalRectangle, int zIndex, const glm::vec4& color, Texture2D* texture, Material* customMaterial) {
    UiRenderNode node;
    node.finalRectangle = finalRectangle;
    node.zIndex = zIndex;
    node.color = color;
    node.texture = texture;
    node.customMaterial = customMaterial;

    EnqueueUi(node);
}

void SceneGraphics::DrawUiText(const glm::vec4& finalRectangle, int zIndex, const glm::vec4& color, Texture2D* texture, const glm::vec4& uvRectangle, float pxRange) {
    UiRenderNode node;
    node.finalRectangle = finalRectangle;
    node.zIndex = zIndex;
    node.color = color;
    node.texture = texture;
    node.uvRectangle = uvRectangle;
    node.pxRange = pxRange;
    node.isText = true;

    EnqueueUi(node);
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

    RenderPassType passType = RenderPassType::Color | RenderPassType::DepthPrepass | RenderPassType::Gizmos | RenderPassType::Transparent | RenderPassType::Volumetric;
    if (this->ssaoSettings.enabled) {
        passType = passType | RenderPassType::SSAO;
    }

	RenderCamera(this->mainCamera, this->mainViewport, RenderParams {
        passType,
		glm::vec4(
			0,
			0,
			this->mainViewport->GetSize()
		),
		false
	});

	this->mainViewport->GetFramebuffer()->Apply();

	glViewport(0, 0, this->mainViewport->GetSize().x, this->mainViewport->GetSize().y);

	RenderCamera(this->mainCamera, this->mainViewport, RenderParams {
		(RenderPassType)(RenderPassType::PostProcessing | RenderPassType::UI),
		glm::vec4(
			0,
			0,
			this->mainViewport->GetSize()
		),
		false
	});

	RenderFullscreenFrameQuad();

	this->opaqueRenders.clear();
	this->transparentRenders.clear();
	this->oitTransparentRenders.clear();
	this->gizmoRenders.clear();
	this->volumetricRenders.clear();
    this->uiRenders.clear();
}

void SceneGraphics::EnqueueOpaque(const RenderNode& node) {
	this->opaqueRenders.push_back(node);

	std::sort(this->opaqueRenders.begin(), this->opaqueRenders.end());
}
void SceneGraphics::EnqueueGizmo(const RenderNode& node) {
	this->gizmoRenders.push_back(node);

	std::sort(this->gizmoRenders.begin(), this->gizmoRenders.end());
}
void SceneGraphics::EnqueueOrderedTransparent(const RenderNode& node) {
	this->transparentRenders.push_back(node);

	std::sort(this->transparentRenders.begin(), this->transparentRenders.end(), [this](const RenderNode& a, const RenderNode& b) -> bool {
		glm::vec3 cameraPos = this->mainCamera->GlobalTransform().Position();

		float distA = glm::distance(cameraPos, a.bounds.GetCenter());
		float distB = glm::distance(cameraPos, b.bounds.GetCenter());

		return distA < distB;
	});
}
void SceneGraphics::EnqueueOITransparent(const RenderNode& node) {
	this->oitTransparentRenders.push_back(node);

	std::sort(this->oitTransparentRenders.begin(), this->oitTransparentRenders.end());
}
void SceneGraphics::EnqueueAdditive(const RenderNode& node) {
	this->additiveRenders.push_back(node);
}
void SceneGraphics::EnqueueVolumetric(const RenderNode& node) {
	this->volumetricRenders.push_back(node);
}

void SceneGraphics::EnqueueUi(const UiRenderNode& node) {
    this->uiRenders.push_back(node);
}

void SceneGraphics::RenderPrepass(const RenderParams& params, Framebuffer* target) {
	RenderPrepass(this->currentUniforms, params, target);
}
void SceneGraphics::RenderPrepass(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(false);
    target->SetCustomAttachmentEnabled(0, true);
    target->Apply();

	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

	glCullFace(GL_BACK);
	glDepthFunc(GL_LESS);
	glClear(GL_DEPTH_BUFFER_BIT | GL_COLOR_BUFFER_BIT);
			
	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

    const ShaderProgram* currentProgram = nullptr;

	for (auto& render : this->opaqueRenders) {
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

        const ShaderProgram*  targetShader = this->shaders.prepassShader;
        bool isMasked = render.material->GetShader()->HasPragma("alpha_mask");

        // Doesn't support opaque particles in the prepass !
        if (render.jointBufferOffset >= 0) {
            targetShader = isMasked ? this->shaders.prepassAnimatedMaskShader : this->shaders.prepassAnimatedShader;
        } else if (render.material->GetShader()->HasPragma("scatter")) {
            targetShader = isMasked ? this->shaders.prepassScatterMaskShader : this->shaders.prepassScatterShader;
        } else if (render.material->GetShader()->HasPragma("complex_vertex_shader")) {
            targetShader = render.material->GetShader();
        } else if (isMasked) {
            targetShader = this->shaders.prepassMaskShader;
        }

        if (currentProgram != targetShader) {
            currentProgram = targetShader;
            glUseProgram(currentProgram->GetHandle());
        }

        if (isMasked || currentProgram == render.material->GetShader()) {
            render.material->Bind();
        }

        if (render.jointBufferOffset >= 0) {
            int offsetLocation = glGetUniformLocation(currentProgram->GetHandle(), "uBoneOffset");
            if (offsetLocation >= 0) {
                glUniform1i(offsetLocation, render.jointBufferOffset);
            }
        }
		
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
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

    glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, 0);
	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderSSAO(const RenderParams& params, Framebuffer* target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());
    glViewport(0, 0, target->GetSize().x, target->GetSize().y);

	static Mesh* quadMesh = GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj");

	glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());
    
    GLuint shaderHandle = this->shaders.ssaoShader->GetHandle();
    glUseProgram(shaderHandle);

    glUniform1i(glGetUniformLocation(shaderHandle, "depthTex"), 0);
    glUniform1i(glGetUniformLocation(shaderHandle, "normalTex"), 1);
    glUniform1i(glGetUniformLocation(shaderHandle, "noiseTex"), 2);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, this->GetMainFramebuffer()->GetDepthTexture()->GetHandle());
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, this->GetMainFramebuffer()->GetCustomAttachmentTexture(0)->GetHandle());
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, this->ssaoNoiseTexture->GetHandle());

    int kernelLocation = glGetUniformLocation(shaderHandle, "samples");
    glUniform3fv(kernelLocation, 64, &this->ssaoKernel[0][0]);

    glm::vec2 resolution = target->GetSize();
    glUniform2fv(glGetUniformLocation(shaderHandle, "resolution"), 1, &resolution[0]);

    glUniform1i(glGetUniformLocation(shaderHandle, "kernelSize"), this->ssaoSettings.kernelSize);
    glUniform1f(glGetUniformLocation(shaderHandle, "radius"), this->ssaoSettings.radius);
    glUniform1f(glGetUniformLocation(shaderHandle, "bias"), this->ssaoSettings.bias);
    glUniform1f(glGetUniformLocation(shaderHandle, "power"), this->ssaoSettings.power);

    glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glUseProgram(0);
}

void SceneGraphics::RenderSSAOBlur(const RenderParams& params, Framebuffer* target) {
    glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());
    glViewport(0, 0, target->GetSize().x, target->GetSize().y);

    static Mesh* quadMesh = GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj");

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(quadMesh->SubMeshAt(0).GetVertexArrayHandle());
    glUseProgram(this->shaders.ssaoBlurShader->GetHandle());

    glUniform1i(glGetUniformLocation(this->shaders.ssaoBlurShader->GetHandle(), "ssaoTex"), 0);
    glActiveTexture(GL_TEXTURE0);

    glBindTexture(GL_TEXTURE_2D, this->ssaoFramebuffer->GetCustomAttachmentTexture(0)->GetHandle());

    glUniform1i(glGetUniformLocation(this->shaders.ssaoBlurShader->GetHandle(), "blurRange"), this->ssaoSettings.blurRange);
    
    glDrawElements(GL_TRIANGLES, quadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);

    glEnable(GL_DEPTH_TEST);
    glBindVertexArray(0);
    glUseProgram(0);
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

    const ShaderProgram* currentProgram = nullptr;

	for (auto& render : this->opaqueRenders) {
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

        const ShaderProgram* targetShader = this->shaders.depthOnlyShader;
        bool isMasked = render.material->GetShader()->HasPragma("alpha_mask");

        if (render.jointBufferOffset >= 0) {
            targetShader = isMasked ? this->shaders.prepassAnimatedMaskShader : this->shaders.depthOnlyAnimatedShader;
        } else if (render.material->GetShader()->HasPragma("scatter")) {
            targetShader = isMasked ? this->shaders.prepassScatterMaskShader : this->shaders.prepassScatterShader;
        } else if (render.material->GetShader()->HasPragma("complex_vertex_shader")) {
            targetShader = render.material->GetShader();
        } else if (isMasked) {
            targetShader = this->shaders.prepassMaskShader;
        }

        if (currentProgram != targetShader) {
            currentProgram = targetShader;
            glUseProgram(currentProgram->GetHandle());
        }

        if (isMasked || currentProgram == render.material->GetShader()) {
            render.material->Bind();
        }

        if (render.jointBufferOffset >= 0) {
            int offsetLocation = glGetUniformLocation(currentProgram->GetHandle(), "uBoneOffset");
            if (offsetLocation >= 0) {
                glUniform1i(offsetLocation, render.jointBufferOffset);
            }
        }

		glBindBuffer(GL_UNIFORM_BUFFER, objectUniformsBuffer);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(objectUniforms), &objectUniforms, GL_STREAM_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		} else {
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

void SceneGraphics::RenderVolumetric(const RenderParams& params, Framebuffer* target) {
	RenderVolumetric(this->currentUniforms, params, target);
}
void SceneGraphics::RenderVolumetric(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());
	
	glViewport(0, 0, this->volumetricPassFramebuffer->GetSize().x, this->volumetricPassFramebuffer->GetSize().y);

	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glDepthMask(GL_FALSE);
	glEnable(GL_BLEND);
	glBlendEquation(GL_FUNC_ADD);
	glBlendFuncSeparate(GL_ONE, GL_SRC_ALPHA, GL_ZERO, GL_SRC_ALPHA);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_ALWAYS);
	glCullFace(GL_FRONT);

	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	const ShaderProgram* currentProg = nullptr;

	for (auto& render : this->volumetricRenders) {
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
        objectUniforms.Object_InverseModelMatrix = glm::inverse(render.transformation);
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
        if (currentProg) {
            if (int offsetLocation = glGetUniformLocation(currentProg->GetHandle(), "uBoneOffset"); offsetLocation >= 0) {
                glUniform1i(offsetLocation, std::max(0, render.jointBufferOffset));
            }


            glUniform1f(glGetUniformLocation(currentProg->GetHandle(), "resolutionScale"), this->volumetricPassResolutionScale);
        }

		int shadowmaskUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_ShadowMask");

		if (shadowmaskUniformLocation >= 0) {
			glActiveTexture(GL_TEXTURE31);
			glBindTexture(GL_TEXTURE_2D, GetLightSystem()->shadowAtlasFramebuffer->GetDepthTexture()->GetHandle());
			glUniform1i(shadowmaskUniformLocation, 31);
		}

		int depthTexLocation = glGetUniformLocation(render.material->GetShader()->GetHandle(), "depthTex");
		if (depthTexLocation >= 0) {
			glActiveTexture(GL_TEXTURE30);
			glBindTexture(GL_TEXTURE_2D, GetMainFramebuffer()->GetDepthTexture()->GetHandle());
			glUniform1i(depthTexLocation, 30);
		}

		glActiveTexture(GL_TEXTURE0);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		} else {
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

	glDepthFunc(GL_LESS);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void SceneGraphics::RenderOpaque(const RenderParams& params, Framebuffer* target) {
	RenderOpaque(this->currentUniforms, params, target);
}
void SceneGraphics::RenderOpaque(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
    target->SetCustomAttachmentEnabled(0, false);
    target->Apply();

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

	for (auto& render : this->opaqueRenders) {
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
        if (currentProg) {
            if (int offsetLocation = glGetUniformLocation(currentProg->GetHandle(), "uBoneOffset"); offsetLocation >= 0) {
                glUniform1i(offsetLocation, std::max(0, render.jointBufferOffset));
            }
        }

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

        int aoMapUniformLocation = glGetUniformLocation(currentProg->GetHandle(), "Builtin_AOMap");
        if (aoMapUniformLocation >= 0) {
            glActiveTexture(GL_TEXTURE27);
            
            if (this->ssaoSettings.enabled) {
                glBindTexture(GL_TEXTURE_2D, this->ssaoBlurFramebuffer->GetCustomAttachmentTexture(0)->GetHandle());
            } else {
                static Texture2D* fallbackTexture = this->GetScene()->Resources()->Get<Texture2D>("./res/textures/default_color.png", Texture::ColorTextureRGB);
                glBindTexture(GL_TEXTURE_2D, fallbackTexture->GetHandle());
            }
            glUniform1i(aoMapUniformLocation, 27);
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		} else {
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

void SceneGraphics::RenderOrderedTransparent(const RenderParams& params, Framebuffer* target) {
	RenderOrderedTransparent(this->currentUniforms, params, target);
}
void SceneGraphics::RenderOrderedTransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);
	
	glCullFace(GL_BACK);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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
        if (currentProg) {
            if (int offsetLocation = glGetUniformLocation(currentProg->GetHandle(), "uBoneOffset"); offsetLocation >= 0) {
                glUniform1i(offsetLocation, std::max(0, render.jointBufferOffset));
            }
        }

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

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		} else {
			if (render.instanceCount <= 0) {
				glDrawElements(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(render.mesh->GetDrawMode(), render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		}
	}

	glDisable(GL_BLEND);
	glDepthMask(true);
}

void SceneGraphics::RenderOITransparent(const RenderParams& params, Framebuffer* target) {
	RenderOITransparent(this->currentUniforms, params, target);
}
void SceneGraphics::RenderOITransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);
	
	glCullFace(GL_BACK);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunci(0, GL_ONE, GL_ONE);
	glBlendFunci(1, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
	glBlendEquation(GL_FUNC_ADD);

	glClearBufferfv(GL_COLOR, 0, &glm::zero<glm::vec4>()[0]); 
	glClearBufferfv(GL_COLOR, 1, &glm::one<glm::vec4>()[0]);

	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	const ShaderProgram* currentProg = nullptr;

	for (auto& render : this->oitTransparentRenders) {
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
        if (currentProg) {
            if (int offsetLocation = glGetUniformLocation(currentProg->GetHandle(), "uBoneOffset"); offsetLocation >= 0) {
                glUniform1i(offsetLocation, std::max(0, render.jointBufferOffset));
            }
        }

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

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		} else {
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
	glDepthMask(true);
	glDisable(GL_BLEND);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	glBindVertexArray(0);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void SceneGraphics::RenderAdditive(const RenderParams& params, Framebuffer* target) {
	RenderAdditive(this->currentUniforms, params, target);
}
void SceneGraphics::RenderAdditive(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
	target->SetColorAttachmentEnabled(true);
	glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

	glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);
	
	glCullFace(GL_BACK);
	glDepthMask(false);
	glEnable(GL_BLEND);
	glBlendFunci(0, GL_ONE, GL_ONE);
	glBlendEquation(GL_FUNC_ADD);

	ShaderObjectUniforms objectUniforms;

	Frustum viewFrustum = ComputeFrustum(uniforms.Global_VPMatrix);

	const ShaderProgram* currentProg = nullptr;

	for (auto& render : this->additiveRenders) {
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
        if (currentProg) {
            if (int offsetLocation = glGetUniformLocation(currentProg->GetHandle(), "uBoneOffset"); offsetLocation >= 0) {
                glUniform1i(offsetLocation, std::max(0, render.jointBufferOffset));
            }
        }

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

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			if (render.instanceCount <= 0) {
				glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
			}
			else {
				glDrawElementsInstanced(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr, render.instanceCount);
			}
		} else {
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
	glDepthMask(true);
	glDisable(GL_BLEND);
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

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, render.instanceSSBO);
		glBindVertexArray(render.mesh->GetVertexArrayHandle());

        if (render.isIndirect) {
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, render.indirectBuffer);

            void* offsetPointer = (void*)(uintptr_t)render.indirectBufferOffset;

            if (render.material->GetShader()->UsesPatches()) {
                glPatchParameteri(GL_PATCH_VERTICES, (int)render.mesh->GetType());
                glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, offsetPointer);
            } else {
                glDrawElementsIndirect(render.mesh->GetDrawMode(), GL_UNSIGNED_INT, offsetPointer);
            }
            glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
        }
        else if (render.material->GetShader()->UsesPatches()) {
			glPatchParameteri(GL_PATCH_VERTICES, (int) render.mesh->GetType());

			glDrawElements(GL_PATCHES, render.mesh->GetVertexCount(), GL_UNSIGNED_INT, nullptr);
		} else {
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

    Framebuffer* mainFramebuffer = GetMainFramebuffer();
	Framebuffer* ping = mainFramebuffer;
	Framebuffer* pong = postProcess->GetPostProcessBuffer();
	
	Texture2D* frameDepth = dynamic_cast<Texture2D*>(GetMainFramebuffer()->GetDepthTexture());

	for (auto* effect : postProcess->IterateObjects()) {
		PostProcessParams postProcessParams;
		postProcessParams.inputTexture = dynamic_cast<Texture2D*>(ping->GetColorTexture());
		postProcessParams.outputTexture = dynamic_cast<Texture2D*>(pong->GetColorTexture());
		postProcessParams.depthTexture = frameDepth;

		effect->OnPostProcess(&postProcessParams);

		std::swap(ping, pong);
	}

	if (ping != mainFramebuffer) {
		glCopyImageSubData(
			ping->GetColorTexture()->GetHandle(),
			GL_TEXTURE_2D, 0, 0, 0, 0,
			mainFramebuffer->GetColorTexture()->GetHandle(),
			GL_TEXTURE_2D, 0, 0, 0, 0,
			this->mainViewport->GetSize().x,
			this->mainViewport->GetSize().y,
			1
		);
	}
}

void SceneGraphics::RenderUi(const RenderParams& params, Framebuffer* target) {
    RenderUi(this->currentUniforms, params, target);
}

void SceneGraphics::RenderUi(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target) {
    std::sort(this->uiRenders.begin(), this->uiRenders.end());

    target->SetColorAttachmentEnabled(true);
    glBindFramebuffer(GL_FRAMEBUFFER, target->GetHandle());

    glViewport(params.viewport.x, params.viewport.y, params.viewport.z, params.viewport.w);

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDisable(GL_CULL_FACE);

    glm::mat4 projection = glm::ortho(0.0f, params.viewport.z, params.viewport.w, 0.0f, -1.0f, 1.0f);
    glBindVertexArray(this->uiQuadMesh->SubMeshAt(0).GetVertexArrayHandle());

    const ShaderProgram* currentProgram = nullptr;

    for (const auto& render : this->uiRenders) {
        const ShaderProgram* targetProgram = render.isText ? this->shaders.uiTextShader : (render.customMaterial ? render.customMaterial->GetShader() : this->shaders.uiShader);

        if (currentProgram != targetProgram) {
            currentProgram = targetProgram;
            glUseProgram(currentProgram->GetHandle());
        }
        
        if (render.customMaterial) {
            render.customMaterial->Bind();
        }

        int projectionLocation = glGetUniformLocation(currentProgram->GetHandle(), "projection");
        if (projectionLocation >= 0) {
            glUniformMatrix4fv(projectionLocation, 1, GL_FALSE, &projection[0][0]);
        }

        glm::mat4 model = glm::mat4(1.0f);
        model = glm::translate(model, glm::vec3(render.finalRectangle.x, render.finalRectangle.y, 0.0f));
        model = glm::scale(model, glm::vec3(render.finalRectangle.z, render.finalRectangle.w, 1.0f));

        int modelLocation = glGetUniformLocation(currentProgram->GetHandle(), "model");
        if (modelLocation >= 0) {
            glUniformMatrix4fv(modelLocation, 1, GL_FALSE, &model[0][0]);
        }

        if (render.isText) {
            if (int uvRectangleLocation = glGetUniformLocation(currentProgram->GetHandle(), "uvRectangle"); uvRectangleLocation >= 0) {
                glUniform4fv(uvRectangleLocation, 1, &render.uvRectangle[0]);
            }
            if (int pxRangeLocation = glGetUniformLocation(currentProgram->GetHandle(), "pxRange"); pxRangeLocation >= 0) {
                glUniform1f(pxRangeLocation, render.pxRange);
            }
            if (int colorLocation = glGetUniformLocation(currentProgram->GetHandle(), "textColor"); colorLocation >= 0) {
                glUniform4fv(colorLocation, 1, &render.color[0]);
            }
            if (render.texture) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, render.texture->GetHandle());
                if (int texLoc = glGetUniformLocation(currentProgram->GetHandle(), "msdfAtlas"); texLoc >= 0) {
                    glUniform1i(texLoc, 0);
                }
            }
        } else if (!render.customMaterial) {
            int colorLocation = glGetUniformLocation(currentProgram->GetHandle(), "color");
            int hasTextureLocation = glGetUniformLocation(currentProgram->GetHandle(), "hasTexture");
            int textureLocation = glGetUniformLocation(currentProgram->GetHandle(), "tex");

            if (colorLocation >= 0) glUniform4fv(colorLocation, 1, &render.color[0]);
            
            if (render.texture) {
                if (hasTextureLocation >= 0) glUniform1i(hasTextureLocation, 1);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, render.texture->GetHandle());
                if (textureLocation >= 0) glUniform1i(textureLocation, 0);
            } else {
                if (hasTextureLocation >= 0) glUniform1i(hasTextureLocation, 0);
            }
        }

        glDrawElements(GL_TRIANGLES, this->uiQuadMesh->SubMeshAt(0).GetVertexCount(), GL_UNSIGNED_INT, nullptr);
    }

    glBindVertexArray(0);
    glUseProgram(0);
    glEnable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);
    glEnable(GL_CULL_FACE);
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
    this->currentUniforms.Global_InverseViewMatrix = glm::inverse(camera->ViewMatrix());
	this->currentUniforms.Global_ProjectionMatrix = camera->ProjectionMatrix();
    this->currentUniforms.Global_InverseProjectionMatrix = glm::inverse(camera->ProjectionMatrix());
	this->currentUniforms.Global_VPMatrix = this->currentUniforms.Global_ProjectionMatrix * this->currentUniforms.Global_ViewMatrix;
	this->currentUniforms.Global_CameraWorldPos = glm::vec4(camera->GlobalTransform().Position().Value(), 0.0);
	this->currentUniforms.Global_Time = Time::Current();
	this->currentUniforms.Global_CameraFarPlane = camera->GetFarPlane();
	this->currentUniforms.Global_CameraNearPlane = camera->GetNearPlane();
	this->currentUniforms.Global_CameraFov = camera->GetFovRad();
    this->currentUniforms.Global_Resolution = glm::vec4(this->mainViewport->GetSize().x, this->mainViewport->GetSize().y, 0.0f, 0.0f);

	RenderParams activeParams((RenderPassType) 0, params.viewport, false, camera->GetLayerMask());

	BindUniformBuffers();

	if ((params.pass & RenderPassType::DepthPrepass) == RenderPassType::DepthPrepass) {
		activeParams.clearDepth = true;
		activeParams.pass = RenderPassType::DepthPrepass;

		RenderPrepass(activeParams, renderTarget->GetFramebuffer());
	}

    if ((params.pass & RenderPassType::SSAO) == RenderPassType::SSAO) {
        activeParams.clearDepth = false;
        activeParams.pass = RenderPassType::SSAO;

        RenderSSAO(activeParams, this->ssaoFramebuffer);

        RenderSSAOBlur(activeParams, this->ssaoBlurFramebuffer);
    }

	if ((params.pass & RenderPassType::Color) == RenderPassType::Color) {
		activeParams.pass = RenderPassType(RenderPassType::Color);
	
		RenderOpaque(activeParams, renderTarget->GetFramebuffer());
	}

	if (camera == this->mainCamera && (params.pass & RenderPassType::Volumetric) == RenderPassType::Volumetric) {
		activeParams.pass = RenderPassType(RenderPassType::Volumetric);

		RenderVolumetric(activeParams, this->volumetricPassFramebuffer);

		CompositeVolumetricPass();
    }

	if ((params.pass & RenderPassType::Gizmos) == RenderPassType::Gizmos) {
		activeParams.pass = RenderPassType(RenderPassType::Gizmos);
	
		RenderGizmos(activeParams, renderTarget->GetFramebuffer());
	}

	if (camera == this->mainCamera && (params.pass & RenderPassType::Transparent) == RenderPassType::Transparent) {
		activeParams.pass = RenderPassType(RenderPassType::Transparent);
	
		RenderOrderedTransparent(activeParams, renderTarget->GetFramebuffer());
		RenderOITransparent(activeParams, this->transparentPassFramebuffer);

		CompositeTransparentPass();
	}

	if ((params.pass & RenderPassType::Additive) == RenderPassType::Additive) {
		activeParams.pass = RenderPassType(RenderPassType::Additive);
	
		RenderAdditive(activeParams, renderTarget->GetFramebuffer());
	}

	if ((params.pass & RenderPassType::PostProcessing) == RenderPassType::PostProcessing) {
		RenderPostprocess();
	}

    if ((params.pass & RenderPassType::UI) == RenderPassType::UI) {
        activeParams.pass = RenderPassType(RenderPassType::UI);
        RenderUi(activeParams, renderTarget->GetFramebuffer());
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

        // SSAO
        if (ImGui::TreeNode("SSAO Settings")) {
            ImGui::Checkbox("Enable SSAO", &this->ssaoSettings.enabled);

            ImGui::BeginDisabled(!this->ssaoSettings.enabled);
            ImGui::SliderInt("Kernel Size", &this->ssaoSettings.kernelSize, 8, 64);
            ImGui::SliderFloat("Radius", &this->ssaoSettings.radius, 0.1f, 5.0f);
            ImGui::SliderFloat("Bias", &this->ssaoSettings.bias, 0.001f, 0.1f, "%.4f");
            ImGui::SliderFloat("Power", &this->ssaoSettings.power, 0.1f, 5.0f);
            ImGui::SliderInt("Blur Range", &this->ssaoSettings.blurRange, 1, 4);

            // Scale 
            const char* scaleNames[] = { "100%", "75%", "50%", "25%" };
            float scaleValues[] = { 1.0f, 0.75f, 0.5f, 0.25f };

            int currentScaleIndex = 0;
            if (this->ssaoSettings.resolutionScale <= 0.25f) currentScaleIndex = 3;
            else if (this->ssaoSettings.resolutionScale <= 0.5f) currentScaleIndex = 2;
            else if (this->ssaoSettings.resolutionScale <= 0.75f) currentScaleIndex = 1;

            if (ImGui::Combo("Resolution Scale", &currentScaleIndex, scaleNames, 4)) {
                this->ssaoSettings.resolutionScale = scaleValues[currentScaleIndex];
                
                glm::vec2 ssaoResolution = glm::ceil(this->GetScreenResolution() * this->ssaoSettings.resolutionScale);
                this->ssaoFramebuffer->SetSize(ssaoResolution);
                this->ssaoBlurFramebuffer->SetSize(ssaoResolution);
            }

            if (ImGui::Button("Reset")) {
                ssaoSettings = {};
            }

            ImGui::EndDisabled();

            ImGui::TreePop();
        }
        // Volumetric
        if (ImGui::TreeNode("Volumetric Settings")) {
            const char* scaleNames[] = { "100%", "75%", "50%", "25%" };
            float scaleValues[] = { 1.0f, 0.75f, 0.5f, 0.25f };

            int currentScaleIndex = 0;
            if (this->volumetricPassResolutionScale <= 0.25f) currentScaleIndex = 3;
            else if (this->volumetricPassResolutionScale <= 0.5f) currentScaleIndex = 2;
            else if (this->volumetricPassResolutionScale <= 0.75f) currentScaleIndex = 1;

            if (ImGui::Combo("Resolution Scale", &currentScaleIndex, scaleNames, 4)) {
                this->volumetricPassResolutionScale = scaleValues[currentScaleIndex];

                glm::vec2 volumetricResolution = glm::ceil(this->GetScreenResolution() * this->volumetricPassResolutionScale);
                this->volumetricPassFramebuffer->SetSize(volumetricResolution);
            }

            ImGui::TreePop();
        } 

		ImGui::TreePop();
	}
}

int SceneGraphics::Order() {
	return INT_MAX;
}

// Based on learnopengl.com
void SceneGraphics::GenerateSSAOKernelAndTexture() {
    // Kernel
    std::uniform_real_distribution<float> randomFloats(0.0, 1.0); // random floats between [0.0, 1.0]
    std::default_random_engine generator;
    for (unsigned int i = 0; i < 64; ++i)
    {
        glm::vec3 sample(
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator) * 2.0 - 1.0, 
            randomFloats(generator)
        );
        sample  = glm::normalize(sample);
        sample *= randomFloats(generator);
        float scale = float(i) / 64.0f;

        scale = std::lerp(0.1f, 1.0f, scale * scale);
        sample *= scale;
        this->ssaoKernel.push_back(sample);
    }

    // Texture
    std::vector<glm::vec3> ssaoNoise;
    for (unsigned int i = 0; i < 16; i++) {
        glm::vec3 noise(randomFloats(generator) * 2.0 - 1.0, randomFloats(generator) * 2.0 - 1.0, 0.0f);
        ssaoNoise.push_back(noise);
    }

    TextureParams params = TextureParams {
        .channels = TextureChannels::RGB,
        .colorSpace = TextureColor::Linear,
        .format = TextureFormat::Float32,
        .wrapU = TextureWrap::Repeat,
        .wrapV = TextureWrap::Repeat,
        .minFilter = TextureFilter::Nearest,
        .magFilter = TextureFilter::Nearest,
    };
    this->ssaoNoiseTexture.reset(Texture2D::Create(
        reinterpret_cast<unsigned char*>(ssaoNoise.data()),
        4, 4,
        params
    ));
}

void SceneGraphics::SetupShaders() {
    // Added a shader for skinned meshes because i needed one for the prepass either way
    this->shaders.depthOnlyShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/basic.vert")
        .WithPixelShader("./res/shaders/basic.frag")
        .Link();
    this->shaders.depthOnlyAnimatedShader= ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_animated.vert")
        .WithPixelShader("./res/shaders/basic.frag")
        .Link();

    // None of these use normal maps
    // also shaders for materials using alpha mask are missing
	this->shaders.prepassShader = ShaderProgram::Build()
	    .WithVertexShader("./res/shaders/prepass/prepass.vert")
	    .WithPixelShader("./res/shaders/prepass/prepass.frag")
	    .Link();
    this->shaders.prepassAnimatedShader= ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_animated.vert")
        .WithPixelShader("./res/shaders/prepass/prepass.frag")
        .Link();
    // No support for opaque particles
    this->shaders.prepassScatterShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_scatter.vert")
        .WithPixelShader("./res/shaders/prepass/prepass.frag")
        .Link();
    this->shaders.prepassMaskShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass.vert")
        .WithPixelShader("./res/shaders/prepass/prepass_mask.frag")
        .Link();

    this->shaders.prepassAnimatedMaskShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_animated.vert")
        .WithPixelShader("./res/shaders/prepass/prepass_mask.frag")
        .Link();

    this->shaders.prepassScatterMaskShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_scatter.vert")
        .WithPixelShader("./res/shaders/prepass/prepass_mask.frag")
        .Link();

    // SSAO
    this->shaders.ssaoShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/ssao/ssao.vert")
        .WithPixelShader("./res/shaders/ssao/ssao.frag")
        .Link();
    this->shaders.ssaoBlurShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/ssao/ssao.vert")
        .WithPixelShader("./res/shaders/ssao/ssao_blur.frag")
        .Link();

    // UI
    this->shaders.uiShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/ui/ui.vert")
        .WithPixelShader("./res/shaders/ui/ui.frag")
        .Link();
    this->shaders.uiTextShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/ui/ui_text.vert")
        .WithPixelShader("./res/shaders/ui/ui_text.frag")
        .Link();
}
