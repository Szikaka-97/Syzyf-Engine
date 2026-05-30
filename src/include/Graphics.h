#pragma once

#include <glm/fwd.hpp>
#include <optional>
#include <queue>
#include <vector>
#include <glad/glad.h>
#include <glm/glm.hpp>

#include <Mesh.h>
#include <Material.h>
#include <Camera.h>
#include <Framebuffer.h>
#include <GameObjectSystem.h>
#include <Layer.h>

#include "../res/shaders/shared/uniforms.h"

struct ShaderGlobalUniforms;
class MeshRenderer;
class Scene;
class ComputeShaderDispatch;
class Texture2D;
class LightSystem;
class PostProcessingSystem;
class ReflectionProbeSystem;
class Camera;
class Viewport;

struct RenderParams {
	RenderPassType pass;
	glm::vec4 viewport;
	bool clearDepth;
	LayerMask layers;

	RenderParams(RenderPassType pass, glm::vec4 viewport, bool clearDepth = false, LayerMask layers = LayerMask::All);
};

class SceneGraphics : public GameObjectSystem<Camera> {
public:
    struct SSAOSettings {
        bool enabled = true;
        // max kernel size is hardcoded to 64
        int kernelSize = 32;
        float radius = 1.5f;
        float bias = 0.025f;
        float power = 4.0f;
        int blurRange = 2;
        // Should be private but i dnt care
        float resolutionScale = 1.0f;
    };

    SSAOSettings ssaoSettings;
private:
    // this should all be using unique ptrs
    struct Shaders {
        // Depth
        ShaderProgram* depthOnlyShader;
        ShaderProgram* depthOnlyAnimatedShader;
        // Prepass
        ShaderProgram* prepassShader;
        ShaderProgram* prepassAnimatedShader;
        ShaderProgram* prepassScatterShader;
        ShaderProgram* prepassMaskShader;
        ShaderProgram* prepassDitherHoleShader;
        ShaderProgram* prepassDitherProximityShader;
        ShaderProgram* prepassAnimatedMaskShader;
        ShaderProgram* prepassScatterMaskShader;

        // SSAO 
        ShaderProgram* ssaoShader;
        ShaderProgram* ssaoBlurShader;

        // Mask
        ShaderProgram* maskShader;
        // UI
        ShaderProgram* uiShader;
        ShaderProgram* uiTextShader;
    };

	struct RenderNode {
		const Mesh::SubMesh* mesh;
		const Material* material;
		union {
			unsigned int instanceCount;
			bool ignoreDepth;
		};
		glm::mat4 transformation;
		BoundingBox bounds;
		uint8_t layer;

        int jointBufferOffset = -1;

        uint8_t maskFlags = 0;

        GLuint indirectBuffer = 0;
        GLuint indirectBufferOffset = 0;
        GLuint instanceSSBO = 0;
        bool isIndirect = false;

        RenderNode(const Mesh::SubMesh* mesh, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer, unsigned int instanceCount = 0, GLuint instanceSSBO = 0, bool ignoreDepth = false);
        RenderNode(const Mesh::SubMesh* mesh, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer, GLuint indirectBuffer, GLuint indirectBufferOffset, GLuint instanceSSBO);

		bool operator<(const RenderNode& other) const;
	};

    struct UiRenderNode {
        glm::mat4 worldMatrix;
        glm::vec2 size;
        int zIndex = 0;
        glm::vec4 color{1.0f};
        Texture2D* texture = nullptr;
        Material* customMaterial = nullptr;

        bool isText = false;
        bool useMsdf = false;
        std::optional<glm::vec4> clipRectangle;

        glm::vec4 uvRectangle{0.0f, 0.0f, 1.0f, 1.0f};
        float pxRange = 4.0f;

        bool operator<(const UiRenderNode& other) const {
            return zIndex < other.zIndex;
        }
    };

	std::vector<RenderNode> opaqueRenders;
	std::vector<RenderNode> gizmoRenders;
	std::vector<RenderNode> transparentRenders;
	std::vector<RenderNode> oitTransparentRenders;
	std::vector<RenderNode> additiveRenders;
	std::vector<RenderNode> volumetricRenders;
    std::vector<RenderNode> maskRenders;
    std::vector<UiRenderNode> uiRenders;
	GLuint globalUniformsBuffer;
	GLuint objectUniformsBuffer;
	
	Viewport* mainViewport;
	Framebuffer* opaquePassFramebuffer;
	Framebuffer* transparentPassFramebuffer;
	Framebuffer* volumetricPassFramebuffer;
    Framebuffer* ssaoFramebuffer;
    Framebuffer* ssaoBlurFramebuffer;
    Framebuffer* maskFramebuffer;
    float depthMult = 1.0f;

	LightSystem* lightSystem;
	PostProcessingSystem* postProcessing;
	ReflectionProbeSystem* envMapping;

	Camera* mainCamera;

	ShaderGlobalUniforms currentUniforms;
    Shaders shaders;

    std::vector<glm::vec3> ssaoKernel;
    std::unique_ptr<Texture2D> ssaoNoiseTexture;

    float volumetricPassResolutionScale = 1.0f;

    Mesh* uiQuadMesh;

	void RenderFullscreenFrameQuad();
	void CompositeTransparentPass();
	void CompositeVolumetricPass();
	
	void RenderPostprocess();

	void Render();

	void EnqueueOpaque(const RenderNode& node);
	void EnqueueGizmo(const RenderNode& node);
	void EnqueueOrderedTransparent(const RenderNode& node);
	void EnqueueOITransparent(const RenderNode& node);
	void EnqueueAdditive(const RenderNode& node);
	void EnqueueVolumetric(const RenderNode& node);
    void EnqueueUi(const UiRenderNode& node);

    void EnqueueMask(const RenderNode& node);

	void BindMaterialProperties(Material* mat);

    void GenerateSSAOKernelAndTexture();
    void SetupShaders();
public:
	SceneGraphics(Scene* scene);
	
    void SetSSAOEnabled(bool enabled);

	glm::vec2 GetScreenResolution() const;
	void UpdateScreenResolution(glm::vec2 newResolution);
	
	LightSystem* GetLightSystem();
	PostProcessingSystem* GetPostProcessing();
	ReflectionProbeSystem* GetEnvMapping();
	
	Viewport* GetMainViewport() const;
	Framebuffer* GetMainFramebuffer() const;
	
	Camera* GetMainCamera() const;
	void SetMainCamera(Camera* camera);

	void BindUniformBuffers();
	
	void DrawMesh(MeshRenderer* renderer);
	void DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, uint8_t layer = Layer::Default);
	void DrawMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, const BoundingBox& bounds, uint8_t layer = Layer::Default);
	
    void DrawMeshInstanced(MeshRenderer* renderer, unsigned int instanceCount);
    void DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, GLuint instanceSSBO = 0, uint8_t layer = Layer::Default);
    void DrawMeshInstanced(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, unsigned int instanceCount, const BoundingBox& bounds, GLuint instanceSSBO = 0, uint8_t layer = Layer::Default);

    void DrawMeshIndirect(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, GLuint indirectBuffer, GLuint indirectBufferOffset, GLuint instanceSSBO, const BoundingBox& bounds, uint8_t layer = Layer::Default);

    void DrawUi(const glm::mat4& worldMatrix, const glm::vec2& size, int zIndex, const glm::vec4& color, Texture2D* texture = nullptr, Material* customMaterial = nullptr, std::optional<glm::vec4> clipRectangle = std::nullopt);
    void DrawUiText(const glm::mat4& worldMatrix, const glm::vec2& size, int zIndex, const glm::vec4& color, Texture2D* texture, const glm::vec4& uvRectangle, float pxRange, bool useMsdf = true, std::optional<glm::vec4> clipRectangle = std::nullopt);

	void DrawGizmoMesh(const Mesh* mesh, int subMeshIndex, const Material* material, const glm::mat4& transformation, bool ignoresDepth = false);
	
	void RenderCamera(Camera* camera, Viewport* renderTarget = nullptr);
	void RenderCamera(Camera* camera, const RenderParams& params);
	void RenderCamera(Camera* camera, Viewport* renderTarget, const RenderParams& params);
	
	void RenderPrepass(const RenderParams& params, Framebuffer* target);
	void RenderPrepass(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderSSAO(const RenderParams& params, Framebuffer* target);
	void RenderSSAO(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderSSAOBlur(const RenderParams& params, Framebuffer* target);
	void RenderSSAOBlur(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

    void RenderMask(const RenderParams& params, Framebuffer* target);
    void RenderMask(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderOpaque(const RenderParams& params, Framebuffer* target);
	void RenderOpaque(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderOrderedTransparent(const RenderParams& params, Framebuffer* target);
	void RenderOrderedTransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderOITransparent(const RenderParams& params, Framebuffer* target);
	void RenderOITransparent(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderAdditive(const RenderParams& params, Framebuffer* target);
	void RenderAdditive(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderGizmos(const RenderParams& params, Framebuffer* target);
	void RenderGizmos(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	void RenderShadows(const RenderParams& params, Framebuffer* target);
	void RenderShadows(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);
	
	void RenderVolumetric(const RenderParams& params, Framebuffer* target);
	void RenderVolumetric(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

    void RenderUi(const RenderParams& params, Framebuffer* target);

    void RenderUi(const ShaderGlobalUniforms& uniforms, const RenderParams& params, Framebuffer* target);

	virtual void OnPostRender();

	virtual void DrawImGui();

	virtual int Order();
};
