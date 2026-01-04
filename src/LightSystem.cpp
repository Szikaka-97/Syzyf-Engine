#include <LightSystem.h>

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>

#include <Light.h>
#include <Camera.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

constexpr int MAX_NUM_LIGHTS = 128;
constexpr int SHADOW_MAP_ATLAS_SIZE = 4096;

constexpr int DIRECTIONAL_LIGHT_CASCADE_COUNT = 6;

LightSystem::LightSystem(Scene* scene):
GameObjectSystem<Light>(scene),
lightsBuffer(0)  {
	glGenFramebuffers(1, &this->shadowAtlasFramebuffer);

	glGenTextures(1, &this->shadowAtlasDepthTexture);

	glBindTexture(GL_TEXTURE_2D, this->shadowAtlasDepthTexture);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_MAP_ATLAS_SIZE, SHADOW_MAP_ATLAS_SIZE, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);

	glBindTexture(GL_TEXTURE_2D, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, this->shadowAtlasFramebuffer);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, this->shadowAtlasDepthTexture, 0);

	glGenBuffers(1, &this->lightsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->lightsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 32 + sizeof(ShaderLightRep) * MAX_NUM_LIGHTS, nullptr, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &this->shadowmapsBuffer);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
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
	
	GetScene()->GetGraphics()->BindGlobalUniformBuffer(globalUniforms);
	
	glViewport(shadowmapRect.start.x, shadowmapRect.start.y, shadowmapRect.end.x - shadowmapRect.start.x, shadowmapRect.end.y - shadowmapRect.start.y);
	
	GetScene()->GetGraphics()->RenderObjects(globalUniforms);

	shadowmapRect.start /= SHADOW_MAP_ATLAS_SIZE;
	shadowmapRect.end /= SHADOW_MAP_ATLAS_SIZE;
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

	for (int cascade = 0; cascade < DIRECTIONAL_LIGHT_CASCADE_COUNT; cascade++) {
		float cascadeFrustumStart = farPlane * (float(cascade) / DIRECTIONAL_LIGHT_CASCADE_COUNT) * (float(cascade) / DIRECTIONAL_LIGHT_CASCADE_COUNT);
		float cascadeFrustumEnd = farPlane * (float(cascade + 1) / DIRECTIONAL_LIGHT_CASCADE_COUNT) * (float(cascade + 1) / DIRECTIONAL_LIGHT_CASCADE_COUNT);

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

		globalUniforms.Global_ProjectionMatrix = glm::ortho(low.x, high.x, low.y, high.y, low.z, high.z);
		globalUniforms.Global_CameraWorldPos = lightPos;
		globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;

		ShadowMapRegion& shadowmapRect = shadowmapRects[cascade];
		
		shadowmapRect.viewTransform = globalUniforms.Global_VPMatrix;
		
		GetScene()->GetGraphics()->BindGlobalUniformBuffer(globalUniforms);
		
		glViewport(shadowmapRect.start.x, shadowmapRect.start.y, shadowmapRect.end.x - shadowmapRect.start.x, shadowmapRect.end.y - shadowmapRect.start.y);
		
		GetScene()->GetGraphics()->RenderObjects(globalUniforms);

		shadowmapRect.start /= SHADOW_MAP_ATLAS_SIZE;
		shadowmapRect.end /= SHADOW_MAP_ATLAS_SIZE;
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
		globalUniforms.Global_ViewMatrix = glm::lookAt(
			light->GlobalTransform().Position().Value(),
			light->GlobalTransform().Position() + directions[face],
			glm::vec3(0, 1, 1)
		);
		globalUniforms.Global_ProjectionMatrix = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, light->GetRange());
		globalUniforms.Global_VPMatrix = globalUniforms.Global_ProjectionMatrix * globalUniforms.Global_ViewMatrix;

		ShadowMapRegion& shadowmapRect = shadowmapRects[face];

		shadowmapRect.viewTransform = globalUniforms.Global_VPMatrix;

		GetScene()->GetGraphics()->BindGlobalUniformBuffer(globalUniforms);
		
		glViewport(shadowmapRect.start.x, shadowmapRect.start.y, shadowmapRect.end.x - shadowmapRect.start.x, shadowmapRect.end.y - shadowmapRect.start.y);
		
		GetScene()->GetGraphics()->RenderObjects(globalUniforms);
	
		shadowmapRect.start /= SHADOW_MAP_ATLAS_SIZE;
		shadowmapRect.end /= SHADOW_MAP_ATLAS_SIZE;
	}	
}

void LightSystem::OnPostRender() {
	glBindFramebuffer(GL_FRAMEBUFFER, this->shadowAtlasFramebuffer);
	glClear(GL_DEPTH_BUFFER_BIT);

	glCullFace(GL_FRONT);

	glm::vec4 ambientLight{1.0, 1.0, 1.0, 0.05};

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
				shadowmapTexturesCount += DIRECTIONAL_LIGHT_CASCADE_COUNT;
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
	
			if (l->IsShadowCasting()) {
				if (l->GetType() == Light::LightType::Spot) {
					rects[shadowMapIndex].start = glm::vec2(xPosition, yPosition);
					rects[shadowMapIndex].end = glm::vec2(xPosition + SHADOW_MAP_ATLAS_SIZE / sizeDivisor, yPosition + SHADOW_MAP_ATLAS_SIZE / sizeDivisor);
	
					xPosition += SHADOW_MAP_ATLAS_SIZE / sizeDivisor;
					if (xPosition >= SHADOW_MAP_ATLAS_SIZE) {
						xPosition = 0;
						yPosition += SHADOW_MAP_ATLAS_SIZE / sizeDivisor;
					}
	
					shadowMapIndex++;
				}
				else if (l->GetType() == Light::LightType::Point) {
					for (int i = 0; i < 6; i++) {
						rects[shadowMapIndex + i].start = glm::vec2(xPosition, yPosition);
						rects[shadowMapIndex + i].end = glm::vec2(xPosition + SHADOW_MAP_ATLAS_SIZE / sizeDivisor, yPosition + SHADOW_MAP_ATLAS_SIZE / sizeDivisor);
	
						xPosition += SHADOW_MAP_ATLAS_SIZE / sizeDivisor;
						if (xPosition >= SHADOW_MAP_ATLAS_SIZE) {
							xPosition = 0;
							yPosition += SHADOW_MAP_ATLAS_SIZE / sizeDivisor;
						}
					}

					shadowMapIndex += 6;
				}
				else if (l->GetType() == Light::LightType::Directional) {
					for (int i = 0; i < DIRECTIONAL_LIGHT_CASCADE_COUNT; i++) {
						rects[shadowMapIndex + i].start = glm::vec2(xPosition, yPosition);
						rects[shadowMapIndex + i].end = glm::vec2(xPosition + SHADOW_MAP_ATLAS_SIZE / sizeDivisor, yPosition + SHADOW_MAP_ATLAS_SIZE / sizeDivisor);
	
						xPosition += SHADOW_MAP_ATLAS_SIZE / sizeDivisor;
						if (xPosition >= SHADOW_MAP_ATLAS_SIZE) {
							xPosition = 0;
							yPosition += SHADOW_MAP_ATLAS_SIZE / sizeDivisor;
						}
					}

					shadowMapIndex += DIRECTIONAL_LIGHT_CASCADE_COUNT;
				}
			}
		}

		shadowMapIndex = 0;
		lightIndex = 0;
		for (const auto& l : *GetAllObjects()) {
			if (lightIndex >= MAX_NUM_LIGHTS) {
				break;
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
						shadowMapIndex += DIRECTIONAL_LIGHT_CASCADE_COUNT;

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

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->lightsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->shadowmapsBuffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, shadowmapTexturesCount * sizeof(ShadowMapRegion), rects, GL_DYNAMIC_DRAW);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->shadowmapsBuffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}