#include <LightSystem.h>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <Light.h>
#include <Camera.h>
#include <Graphics.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

constexpr int MAX_NUM_LIGHTS = 128;

LightSystem::LightSystem(Scene* scene):
GameObjectSystem<Light>(scene),
lightsBuffer(0),
shadowmapAtlasSize(4096),
directionalLightCascadeCount(6),
shadowAtlasDepthTexture(nullptr) {
	this->shadowAtlasFramebuffer = new Framebuffer((Texture2D*) nullptr, 0, nullptr);

	ChangeShadowAtlasResolution(this->shadowmapAtlasSize);

	glGenBuffers(1, &this->lightsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->lightsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 32 + sizeof(ShaderLightRep) * MAX_NUM_LIGHTS, nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &this->shadowmapsBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void LightSystem::ChangeShadowAtlasResolution(int newResolution) {
	if (this->shadowAtlasDepthTexture) {
		delete this->shadowAtlasDepthTexture;
	}

	this->shadowmapAtlasSize = newResolution;

	this->shadowAtlasDepthTexture = new Texture2D(this->shadowmapAtlasSize, this->shadowmapAtlasSize, Texture::DepthBuffer);

	this->shadowAtlasDepthTexture->SetMinFilter(TextureFilter::LinearMipmapNearest);
	this->shadowAtlasDepthTexture->SetMagFilter(TextureFilter::LinearMipmapNearest);

	this->shadowAtlasFramebuffer->SetDepthTexture(this->shadowAtlasDepthTexture, 0);
}

GLuint LightSystem::GetLightsBufferHandle() {
	return this->lightsBuffer;
}
GLuint LightSystem::GetShadowmapsBufferHandle() {
	return this->shadowmapsBuffer;
}

void LightSystem::DoSpotLightShadowmap(Light* light, ShadowMapRegion& shadowmapRect) {
	ShaderGlobalUniforms globalUniforms;
		
	globalUniforms.Global_ViewMatrix = glm::lookAt(
		light->GlobalTransform().Position().Value(),
		light->GlobalTransform().Position() + light->GlobalTransform().Forward(),
		glm::vec3(0, 1, 0)
	);
	globalUniforms.Global_ProjectionMatrix = glm::perspective(light->GetSpotlightAngle() * 2, 1.0f, 0.1f, light->GetRange());
	globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;
	globalUniforms.Global_CameraWorldPos = light->GlobalTransform().Position();
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = 0;
	globalUniforms.Global_CameraNearPlane = 0;
	globalUniforms.Global_CameraFov = 0;

	shadowmapRect.viewTransform = globalUniforms.Global_VPMatrix;
	
	RenderParams renderParams(RenderPassType::Shadows, glm::vec4(
		shadowmapRect.start.x, shadowmapRect.start.y,
		shadowmapRect.end.x - shadowmapRect.start.x, shadowmapRect.end.y - shadowmapRect.start.y
	));

	GetScene()->GetGraphics()->RenderScene(globalUniforms, this->shadowAtlasFramebuffer, renderParams);

	shadowmapRect.start /= this->shadowmapAtlasSize;
	shadowmapRect.end /= this->shadowmapAtlasSize;
}

void LightSystem::DoDirectionalLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects) {
	ShaderGlobalUniforms globalUniforms;
	
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = 0;
	globalUniforms.Global_CameraNearPlane = 0;
	globalUniforms.Global_CameraFov = 0;
	
	Camera* mainCamera = Camera::GetMainCamera();
	
	float nearPlane = mainCamera->GetNearPlane();
	float farPlane = mainCamera->GetFarPlane();
	glm::vec3 cameraPosition = mainCamera->GlobalTransform().Position();
	glm::vec3 cameraForward = mainCamera->GlobalTransform().Forward();

	glm::vec4 frustumCorners[] {
		glm::vec4(-1, -1, -1,  1),
		glm::vec4(-1, -1,  1,  1),
		glm::vec4(-1,  1, -1,  1),
		glm::vec4(-1,  1,  1,  1),
		glm::vec4( 1, -1, -1,  1),
		glm::vec4( 1, -1,  1,  1),
		glm::vec4( 1,  1, -1,  1),
		glm::vec4( 1,  1,  1,  1),
	};

	for (int cascade = 0; cascade < this->directionalLightCascadeCount; cascade++) {
		float cascadeFrustumStart = farPlane * (float(cascade) / this->directionalLightCascadeCount) * (float(cascade) / this->directionalLightCascadeCount);
		float cascadeFrustumEnd = farPlane * (float(cascade + 1) / this->directionalLightCascadeCount) * (float(cascade + 1) / this->directionalLightCascadeCount);

		if (cascadeFrustumStart == 0) {
			cascadeFrustumStart = 0.0001f;
		}

		glm::vec3 frustumCenter = cameraPosition + cameraForward * ((cascadeFrustumStart + cascadeFrustumEnd) * 0.5f);

		globalUniforms.Global_ViewMatrix = glm::lookAt(
			frustumCenter,
			frustumCenter + light->GlobalTransform().Forward(),
			glm::vec3(0, 1, 0)
		);

		glm::mat4 invFrustumMatrix = glm::inverse(glm::perspective(mainCamera->GetFovRad(), mainCamera->GetAspectRatio(), cascadeFrustumStart, cascadeFrustumEnd) * mainCamera->ViewMatrix());

		glm::vec3 low = glm::vec3(INFINITY, INFINITY, INFINITY);
		glm::vec3 high = glm::vec3(-INFINITY, -INFINITY, -INFINITY);

		for (int i = 0; i < 8; i++) {
			glm::vec4 worldSpaceFrustumCorner = invFrustumMatrix * frustumCorners[i];
			worldSpaceFrustumCorner /= worldSpaceFrustumCorner.w;

			glm::vec3 lightSpaceFrustumCorner = globalUniforms.Global_ViewMatrix * worldSpaceFrustumCorner;

			if (lightSpaceFrustumCorner.x > high.x) {
				high.x = lightSpaceFrustumCorner.x;
			}
			if (lightSpaceFrustumCorner.x < low.x) {
				low.x = lightSpaceFrustumCorner.x;
			}
			if (lightSpaceFrustumCorner.y > high.y) {
				high.y = lightSpaceFrustumCorner.y;
			}
			if (lightSpaceFrustumCorner.y < low.y) {
				low.y = lightSpaceFrustumCorner.y;
			}
			if (lightSpaceFrustumCorner.z > high.z) {
				high.z = lightSpaceFrustumCorner.z;
			}
			if (lightSpaceFrustumCorner.z < low.z) {
				low.z = lightSpaceFrustumCorner.z;
			}
		}
		
		glm::vec3 lightPos = frustumCenter;

		globalUniforms.Global_ProjectionMatrix = glm::ortho(low.x, high.x, low.y, high.y, low.z - 100.0f, high.z + 10.0f);
		globalUniforms.Global_CameraWorldPos = lightPos;
		globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;

		ShadowMapRegion& shadowmapRect = shadowmapRects[cascade];
		
		shadowmapRect.viewTransform = globalUniforms.Global_VPMatrix;
		
		RenderParams renderParams(RenderPassType::Shadows, glm::vec4(
			shadowmapRect.start.x, shadowmapRect.start.y,
			shadowmapRect.end.x - shadowmapRect.start.x, shadowmapRect.end.y - shadowmapRect.start.y
		));
		
		GetScene()->GetGraphics()->RenderScene(globalUniforms, this->shadowAtlasFramebuffer, renderParams);

		shadowmapRect.start /= this->shadowmapAtlasSize;
		shadowmapRect.end /= this->shadowmapAtlasSize;
	}
}

void LightSystem::DoPointLightShadowmap(Light* light, ShadowMapRegion* shadowmapRects) {
	const glm::vec3 directions[] {
		{ 1,  0,  0},
		{-1,  0,  0},
		{ 0,  1,  0},
		{ 0, -1,  0},
		{ 0,  0,  1},
		{ 0,  0, -1}
	};

	ShaderGlobalUniforms globalUniforms;
	
	globalUniforms.Global_CameraWorldPos = light->GlobalTransform().Position();
	globalUniforms.Global_Time = (float) glfwGetTime();
	globalUniforms.Global_CameraFarPlane = 0;
	globalUniforms.Global_CameraNearPlane = 0;
	globalUniforms.Global_CameraFov = glm::radians(90.0f);

	for (int face = 0; face < 6; face++) {
		if (face == 2) {
				globalUniforms.Global_ViewMatrix = glm::lookAt(
					light->GlobalTransform().Position().Value(),
					light->GlobalTransform().Position() + directions[face],
					glm::vec3(0, 0, 1)
				);
			}
			else if (face == 3) {
				globalUniforms.Global_ViewMatrix = glm::lookAt(
					light->GlobalTransform().Position().Value(),
					light->GlobalTransform().Position() + directions[face],
					glm::vec3(0, 0, -1)
				);	
			}
			else {
				globalUniforms.Global_ViewMatrix = glm::lookAt(
					light->GlobalTransform().Position().Value(),
					light->GlobalTransform().Position() + directions[face],
					glm::vec3(0, -1, 0)
				);
			}
		globalUniforms.Global_ProjectionMatrix = glm::perspective(glm::radians(90.4f), 1.0f, 0.1f, light->GetRange());
		globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;

		ShadowMapRegion& shadowmapRect = shadowmapRects[face];

		shadowmapRect.viewTransform = globalUniforms.Global_VPMatrix;

		RenderParams renderParams(RenderPassType::Shadows, glm::vec4(
			shadowmapRect.start.x, shadowmapRect.start.y,
			shadowmapRect.end.x - shadowmapRect.start.x, shadowmapRect.end.y - shadowmapRect.start.y
		));
		
		GetScene()->GetGraphics()->RenderScene(globalUniforms, this->shadowAtlasFramebuffer, renderParams);

		shadowmapRect.start /= this->shadowmapAtlasSize;
		shadowmapRect.end /= this->shadowmapAtlasSize;
	}	
}

void LightSystem::OnPostRender() {
	glBindFramebuffer(GL_FRAMEBUFFER, this->shadowAtlasFramebuffer->GetHandle());
	glClear(GL_DEPTH_BUFFER_BIT);

	glm::vec4 ambientLight{1.0, 1.0, 1.0, 0.01};

	int shadowmapTexturesCount = 0;

	int lightIndex = 0;
	for (const auto& l : *GetAllObjects()) {
		if (lightIndex >= MAX_NUM_LIGHTS) {
			break;
		}

		if (l->IsShadowCasting()) {
			if (l->GetType() == Light::LightType::Spot) {
				shadowmapTexturesCount++;
			}
			else if (l->GetType() == Light::LightType::Point) {
				shadowmapTexturesCount += 6;
			}
			else if (l->GetType() == Light::LightType::Directional) {
				shadowmapTexturesCount += this->directionalLightCascadeCount;
			}
		}
	}

	ShadowMapRegion rects[shadowmapTexturesCount];

	int shadowMapIndex = 0;
	int sizeDivisor = 1 << (int) (
		std::ceil(std::log2(std::sqrt(shadowmapTexturesCount)))
	);

	int xPosition = 0;
	int yPosition = 0;
	
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->lightsBuffer);

	if (sizeDivisor > 0) {
		for (const auto& l : *GetAllObjects()) {
			if (lightIndex >= MAX_NUM_LIGHTS) {
				break;
			}

			if (!l->IsEnabled()) {
				continue;
			}
	
			if (l->IsShadowCasting()) {
				if (l->GetType() == Light::LightType::Spot) {
					rects[shadowMapIndex].start = glm::vec2(xPosition, yPosition);
					rects[shadowMapIndex].end = glm::vec2(xPosition + this->shadowmapAtlasSize / sizeDivisor, yPosition + this->shadowmapAtlasSize / sizeDivisor);
	
					xPosition += this->shadowmapAtlasSize / sizeDivisor;
					if (xPosition >= this->shadowmapAtlasSize) {
						xPosition = 0;
						yPosition += this->shadowmapAtlasSize / sizeDivisor;
					}
	
					shadowMapIndex++;
				}
				else if (l->GetType() == Light::LightType::Point) {
					for (int i = 0; i < 6; i++) {
						rects[shadowMapIndex + i].start = glm::vec2(xPosition, yPosition);
						rects[shadowMapIndex + i].end = glm::vec2(xPosition + this->shadowmapAtlasSize / sizeDivisor, yPosition + this->shadowmapAtlasSize / sizeDivisor);
	
						xPosition += this->shadowmapAtlasSize / sizeDivisor;
						if (xPosition >= this->shadowmapAtlasSize) {
							xPosition = 0;
							yPosition += this->shadowmapAtlasSize / sizeDivisor;
						}
					}

					shadowMapIndex += 6;
				}
				else if (l->GetType() == Light::LightType::Directional) {
					for (int i = 0; i < this->directionalLightCascadeCount; i++) {
						rects[shadowMapIndex + i].start = glm::vec2(xPosition, yPosition);
						rects[shadowMapIndex + i].end = glm::vec2(xPosition + this->shadowmapAtlasSize / sizeDivisor, yPosition + this->shadowmapAtlasSize / sizeDivisor);
	
						xPosition += this->shadowmapAtlasSize / sizeDivisor;
						if (xPosition >= this->shadowmapAtlasSize) {
							xPosition = 0;
							yPosition += this->shadowmapAtlasSize / sizeDivisor;
						}
					}

					shadowMapIndex += this->directionalLightCascadeCount;
				}
			}
		}

		shadowMapIndex = 0;
		lightIndex = 0;
		for (const auto& l : *GetAllObjects()) {
			if (lightIndex >= MAX_NUM_LIGHTS) {
				break;
			}

			if (!l->IsEnabled()) {
				continue;
			}
	
			if (l->IsDirty() || l->IsShadowCasting()) {	
				ShaderLightRep rep = l->GetShaderRepresentation();
	
				if (l->IsShadowCasting()) {
					rep.shadowAtlasIndex = shadowMapIndex;
	
					if (l->GetType() == Light::LightType::Spot) {
						shadowMapIndex++;
	
						DoSpotLightShadowmap(l, rects[rep.shadowAtlasIndex]);
					}
					else if (l->GetType() == Light::LightType::Point) {
						shadowMapIndex += 6;
	
						DoPointLightShadowmap(l, rects + rep.shadowAtlasIndex);
					}
					else if (l->GetType() == Light::LightType::Directional) {
						shadowMapIndex += this->directionalLightCascadeCount;

						DoDirectionalLightShadowmap(l, rects + rep.shadowAtlasIndex);
					}
				}
				else {
					rep.shadowAtlasIndex = -1;
				}
		
				glBufferSubData(GL_SHADER_STORAGE_BUFFER, 32 + sizeof(ShaderLightRep) * lightIndex, sizeof(rep), &rep);
			}

			lightIndex++;
		}
	}

	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, sizeof(ambientLight), &ambientLight);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 16, sizeof(lightIndex), &lightIndex);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 20, sizeof(this->directionalLightCascadeCount), &this->directionalLightCascadeCount);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->lightsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->shadowmapsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, shadowmapTexturesCount * sizeof(ShadowMapRegion), rects, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->shadowmapsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

int LightSystem::Order() {
	return 0;
}

void LightSystem::DrawImGui() {
	if (ImGui::TreeNode("Lights Debug")) {
		ImGui::Text("Shadow atlas resolution: %ix%i px", this->shadowmapAtlasSize, this->shadowmapAtlasSize);
		ImGui::Text("Active lights: %i", this->GetAllObjects()->size());

		ImGui::Separator();

		int newShadowmapResolution = this->shadowmapAtlasSize;
	
		ImGui::InputInt("Shadow atlas resolution", &newShadowmapResolution);

		if (ImGui::IsItemDeactivatedAfterEdit() && ImGui::IsKeyPressed(ImGuiKey_Enter)) {
			if (newShadowmapResolution != this->shadowmapAtlasSize) {
				ChangeShadowAtlasResolution(newShadowmapResolution);
			}
		}

		ImGui::TreePop();
	}
}