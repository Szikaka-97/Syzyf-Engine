#include <ReflectionProbeSystem.h>

#include <GLFW/glfw3.h>
#include <glm/glm.hpp>

#include <ReflectionProbe.h>
#include <Graphics.h>

#include "../res/shaders/shared/shared.h"
#include "../res/shaders/shared/uniforms.h"

ReflectionProbeSystem::ReflectionProbeSystem(Scene* scene):
GameObjectSystem<ReflectionProbe>(scene) {
	this->reflectionProbeDepthTexture = new Texture2D(512, 512, TextureFormat::Depth);
	this->reflectionProbeDepthTexture->SetMinFilter(GL_LINEAR);
	this->reflectionProbeDepthTexture->SetMagFilter(GL_LINEAR);
	this->reflectionProbeDepthTexture->SetWrapModeU(GL_CLAMP_TO_EDGE);
	this->reflectionProbeDepthTexture->SetWrapModeV(GL_CLAMP_TO_EDGE);

	this->reflectionProbeFramebuffer = new Framebuffer((Texture2D*) nullptr, 0, this->reflectionProbeDepthTexture, 0);
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

	for (ReflectionProbe* probe : *GetAllObjects()) {
		if (!probe->dirty) {
			continue;
		}

		probe->dirty = false;
		
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

			this->reflectionProbeFramebuffer->SetColorTexture(probe->cubemap, face);

			RenderParams params(RenderPassType::Color, glm::vec4(0, 0, 512, 512), true);
			
			GetScene()->GetGraphics()->RenderScene(globalUniforms, this->reflectionProbeFramebuffer, params);
		}
		
		this->reflectionProbeFramebuffer->SetColorTexture((Texture2D*) nullptr, 0);
	}
}

int ReflectionProbeSystem::Priority() {
	return 50;
}