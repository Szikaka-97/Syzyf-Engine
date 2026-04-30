#include <Graphics.h>

#include <glad/glad.h>
#include <glm/geometric.hpp>
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

    // Added a shader for skinned meshes because i needed one for the prepass either way
    this->depthOnlyShader = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/basic.vert")
        .WithPixelShader("./res/shaders/basic.frag")
        .Link();
    this->depthOnlyShaderAnimated = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_animated.vert")
        .WithPixelShader("./res/shaders/basic.frag")
        .Link();

	this->prepassShader = ShaderProgram::Build()
	    .WithVertexShader("./res/shaders/prepass/prepass.vert")
	    .WithPixelShader("./res/shaders/prepass/prepass.frag")
	    .Link();
    this->prepassShaderAnimated = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_animated.vert")
        .WithPixelShader("./res/shaders/prepass/prepass.frag")
        .Link();
    // No support for opaque particles
    this->prepassShaderScatter = ShaderProgram::Build()
        .WithVertexShader("./res/shaders/prepass/prepass_scatter.vert")
        .WithPixelShader("./res/shaders/prepass/prepass.frag")
        .Link();

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
		this->volumetricPassFramebuffer->SetSize(newResolution * 0.5f);

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
		RenderPassType::Color | RenderPassType::DepthPrepass | RenderPassType::Gizmos | RenderPassType::Transparent | RenderPassType::Volumetric,
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
		RenderPassType::PostProcessing,
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

        const ShaderProgram*  targetShader = this->prepassShader;

        // Doesn't support opaque particles in the prepass !
        if (render.jointBufferOffset >= 0) {
            targetShader = this->prepassShaderAnimated;
        } else if (render.material->GetShader()->HasPragma("scatter")) {
            spdlog::error("Scatter pragma !");
            targetShader = this->prepassShaderScatter;
        } else if (render.material->GetShader()->HasPragma("complex_vertex_shader")) {
            targetShader = render.material->GetShader();
        }

        if (currentProgram != targetShader) {
            currentProgram = targetShader;
            glUseProgram(currentProgram->GetHandle());

            if (currentProgram == render.material->GetShader()) {
                render.material->Bind();
            }
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

        const ShaderProgram* targetShader = this->depthOnlyShader;

        if (render.jointBufferOffset >= 0) {
            targetShader = this->depthOnlyShaderAnimated;
        } else if (render.material->GetShader()->HasPragma("complrex_vertex_shader")) {
            targetShader = render.material->GetShader();
        }

        if (currentProgram != targetShader) {
            currentProgram = targetShader;
            glUseProgram(currentProgram->GetHandle());

            if (currentProgram == render.material->GetShader()) {
                render.material->Bind();
            }
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

	for (auto* effect : *postProcess->GetAllObjects()) {
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
    }}

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
