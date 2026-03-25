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
		BrdfConvolutionDispatch = new ComputeShaderDispatch(ResourceDatabase::Global->Get<ComputeShader>("./res/shaders/cubemapBlit/brdf_convolution.comp"));
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
	this->reflectionProbeFramebuffer = new Framebuffer(Framebuffer::Attachment::HDRColor | Framebuffer::Attachment::CubemappedColor | Framebuffer::Attachment::Depth, ReflectionProbe::resolution, ReflectionProbe::resolution);

	this->brdfConvolutionMap = GenerateBRDFConvolution();
}

void ReflectionProbeSystem::RecalculateSkyboxIBL() {
	// Silly
	Cubemap* skyCubemap = GetScene()->Resources()->Get<Cubemap>("./res/textures/citrus_orchard_road_puresky.hdr", Texture::HDRColorBuffer);

	Skybox* sky = Skybox::GetCurrentSkybox();

	if (this->skyboxProbe == nullptr) {
		this->skyboxProbe = GetScene()->GetRootNode()->AddObject<ReflectionProbe>();
	}

	if (!sky) {
		this->skyboxProbe->dirty = false;
		this->skyboxProbe->irradianceMap = new Cubemap(1, 1, Texture::HDRColorBuffer);
		this->skyboxProbe->prefilterMap = new Cubemap(1, 1, Texture::HDRColorBuffer);

		return;
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
		if (!probe->dirty || !probe->IsEnabled()) {
			continue;
		}

		if (probe == this->skyboxProbe) {
			RecalculateSkyboxIBL();

			break;
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

			this->reflectionProbeFramebuffer->SetColorTexture(this->reflectionProbeFramebuffer->GetColorTexture(), face);

			RenderParams params(RenderPassType::Color, glm::vec4(0, 0, ReflectionProbe::resolution, ReflectionProbe::resolution), true);
			
			GetScene()->GetGraphics()->RenderScene(globalUniforms, this->reflectionProbeFramebuffer, params);
		}
		
		probe->dirty = false;
		probe->irradianceMap = static_cast<Cubemap*>(this->reflectionProbeFramebuffer->GetColorTexture())->GenerateIrradianceMap();
		probe->prefilterMap = static_cast<Cubemap*>(this->reflectionProbeFramebuffer->GetColorTexture())->GeneratePrefilterIBLMap();

		break; // Only recompute 1 probe at a time
	}
}

int ReflectionProbeSystem::Order() {
	return 50;
}

void ReflectionProbeSystem::DrawImGui() {
	if (ImGui::TreeNode("Reflection Probes Debug")) {
		ImGui::Text("Active reflection probes: %i", (int) this->GetAllObjects()->size());

		ReflectionProbe* closest = nullptr;

		if (GetScene()->GetGraphics()->GetMainCamera()) {
			ReflectionProbe* closest = GetClosestProbe(GetScene()->GetGraphics()->GetMainCamera()->GlobalTransform().Position());
		}

		ImGui::Text("Closest probe: %s", closest != nullptr ? closest->GetNode()->GetName().c_str() : "N/A");

		if (ImGui::Button("Invalidate all probes")) {
			InvalidateAll();
		}

		ImGui::TreePop();
	}
}