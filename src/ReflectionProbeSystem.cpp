#include <ReflectionProbeSystem.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <imgui.h>

#include <ReflectionProbe.h>
#include <Graphics.h>
#include <Resources.h>
#include <Skybox.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

Texture2D* GenerateBRDFConvolution() {
	static ComputeShaderDispatch* BrdfConvolutionDispatch;

	if (BrdfConvolutionDispatch == nullptr) {
		BrdfConvolutionDispatch = new ComputeShaderDispatch(Resources::Get<ComputeShader>("./res/shaders/cubemapBlit/brdf_convolution.comp"));
	}

	TextureParams creationParams;
	creationParams.channels = TextureChannels::RG;
	creationParams.format = TextureFormat::Float;
	creationParams.colorSpace = TextureColor::Linear;

	int texSize = 512;

	GLuint handle;

	glCreateTextures(GL_TEXTURE_2D, 1, &handle);
	glBindTexture(GL_TEXTURE_2D, handle);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RG16F, texSize, texSize, 0, GL_RG, GL_FLOAT, nullptr);
	glBindTexture(GL_TEXTURE_2D, 0);

	Texture2D* result = new Texture2D(texSize, texSize, creationParams, handle);

	result->SetWrapModeU(TextureWrap::Clamp);
	result->SetWrapModeV(TextureWrap::Clamp);
	result->SetMinFilter(TextureFilter::Linear);
	result->SetMagFilter(TextureFilter::Linear);
	
	result->Update();

	BrdfConvolutionDispatch->GetData()->SetValue("outputImg", result);
	BrdfConvolutionDispatch->Dispatch(glm::ceil(texSize / 8), glm::ceil(texSize / 8), 1);

	return result;
}

ReflectionProbeSystem::ReflectionProbeSystem(Scene* scene):
GameObjectSystem<ReflectionProbe>(scene),
skyboxProbe(nullptr) {
	this->reflectionProbeDepthTexture = new Texture2D(ReflectionProbe::resolution, ReflectionProbe::resolution, Texture::DepthBuffer);
	this->reflectionProbeColorTexture = new Cubemap(ReflectionProbe::resolution, ReflectionProbe::resolution, Texture::HDRColorBuffer);

	this->reflectionProbeFramebuffer = new Framebuffer(reflectionProbeColorTexture, 0, this->reflectionProbeDepthTexture, 0);

	this->brdfConvolutionMap = GenerateBRDFConvolution();
}

void ReflectionProbeSystem::RecalculateSkyboxIBL() {
	// Silly
	Cubemap* skyCubemap = Resources::Get<Cubemap>("./res/textures/citrus_orchard_road_puresky.hdr", Texture::HDRColorBuffer);

	Skybox* sky = Skybox::GetCurrentSkybox();

	if (!sky) {
		return;
	}

	if (this->skyboxProbe == nullptr) {
		this->skyboxProbe = GetScene()->GetRootNode()->AddObject<ReflectionProbe>();
	}
	
	this->skyboxProbe->dirty = false;
	this->skyboxProbe->irradianceMap = skyCubemap->GenerateIrradianceMap();
	this->skyboxProbe->prefilterMap = skyCubemap->GeneratePrefilterIBLMap();
}

void ReflectionProbeSystem::InvalidateAll() {
	for (ReflectionProbe* probe : *GetAllObjects()) {
		probe->Regenerate();
	}
}

ReflectionProbe* ReflectionProbeSystem::GetClosestProbe(glm::vec3 position) {
	ReflectionProbe* closest = nullptr;
	float closestDistance = INFINITY;

	for (ReflectionProbe* probe : *GetAllObjects()) {
		if (probe->dirty || !probe->IsEnabled() || probe == this->skyboxProbe) {
			continue;
		}

		float dist = glm::distance(probe->GlobalTransform().Position().Value(), position);

		if (dist < closestDistance) {
			closest = probe;
			closestDistance = dist;
		}
	}

	if (closest == nullptr) {
		closest = this->skyboxProbe;
	}

	return closest;
}

Texture2D* ReflectionProbeSystem::BRDFConvolutionMap() {
	return this->brdfConvolutionMap;
}

void ReflectionProbeSystem::OnPostRender() {
	const glm::vec3 directions[] {
		{ 1,  0,  0},
		{-1,  0,  0},
		{ 0,  1,  0},
		{ 0, -1,  0},
		{ 0,  0,  1},
		{ 0,  0, -1}
	};

	if (this->skyboxProbe == nullptr) {
		RecalculateSkyboxIBL();

		return;
	}

	for (ReflectionProbe* probe : *GetAllObjects()) {
		if (!probe->dirty || !probe->IsEnabled() || probe == this->skyboxProbe) {
			continue;
		}

		ShaderGlobalUniforms globalUniforms;
		
		globalUniforms.Global_CameraWorldPos = probe->GlobalTransform().Position();
		globalUniforms.Global_Time = (float) glfwGetTime();
		globalUniforms.Global_CameraFarPlane = 0;
		globalUniforms.Global_CameraNearPlane = 0;
		globalUniforms.Global_CameraFov = glm::radians(90.0f);

		for (int face = 0; face < 6; face++) {
			if (face == 2) {
				globalUniforms.Global_ViewMatrix = glm::lookAt(
					probe->GlobalTransform().Position().Value(),
					probe->GlobalTransform().Position() + directions[face],
					glm::vec3(0, 0, 1)
				);
			}
			else if (face == 3) {
				globalUniforms.Global_ViewMatrix = glm::lookAt(
					probe->GlobalTransform().Position().Value(),
					probe->GlobalTransform().Position() + directions[face],
					glm::vec3(0, 0, -1)
				);	
			}
			else {
				globalUniforms.Global_ViewMatrix = glm::lookAt(
					probe->GlobalTransform().Position().Value(),
					probe->GlobalTransform().Position() + directions[face],
					glm::vec3(0, -1, 0)
				);
			}
			globalUniforms.Global_ProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
			globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;

			this->reflectionProbeFramebuffer->SetColorTexture(this->reflectionProbeColorTexture, face);

			RenderParams params(RenderPassType::Color, glm::vec4(0, 0, ReflectionProbe::resolution, ReflectionProbe::resolution), true);
			
			GetScene()->GetGraphics()->RenderScene(globalUniforms, this->reflectionProbeFramebuffer, params);
		}
		
		this->reflectionProbeFramebuffer->SetColorTexture((Texture2D*) nullptr, 0);

		probe->dirty = false;
		probe->irradianceMap = this->reflectionProbeColorTexture->GenerateIrradianceMap();
		probe->prefilterMap = this->reflectionProbeColorTexture->GeneratePrefilterIBLMap();

		break; // Only recompute 1 probe at a time
	}
}

int ReflectionProbeSystem::Order() {
	return 50;
}

void ReflectionProbeSystem::DrawImGui() {
	if (ImGui::TreeNode("Reflection Probes Debug")) {
		ImGui::Text("Active reflection probes: %i", (int) this->GetAllObjects()->size());

		ReflectionProbe* closest = GetClosestProbe(Camera::GetMainCamera()->GlobalTransform().Position());

		ImGui::Text("Closest probe: %s", closest != nullptr ? closest->GetNode()->GetName().c_str() : "N/A");

		if (ImGui::Button("Invalidate all probes")) {
			InvalidateAll();
		}

		ImGui::TreePop();
	}
}