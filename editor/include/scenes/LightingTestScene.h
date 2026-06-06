#pragma once

#include "ColorGrading.h"
#include "Debug.h"
#include "GameObject.h"
#include "GltfScene.h"
#include "Resources.h"
#include "Shader.h"
#include "Texture.h"
#include "imgui.h"
#include "physics/DebugRenderer.h"
#include "physics/System.h"

#include <Material.h>
#include <Skybox.h>
#include <Light.h>
#include <LightSystem.h>
#include <MeshRenderer.h>
#include <Formatters.h>
#include <Camera.h>
#include <Graphics.h>
#include <InputSystem.h>
#include <ParticleSpawner.h>
#include <Bloom.h>
#include <Tonemapper.h>

#include <glm/ext/scalar_common.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/matrix.hpp>

namespace LightingTestScene {

inline float hue2rgb(float f1, float f2, float hue) {
	if (hue < 0.0)
		hue += 1.0;
	else if (hue > 1.0)
		hue -= 1.0;
	float res;
	if ((6.0 * hue) < 1.0)
		res = f1 + (f2 - f1) * 6.0 * hue;
	else if ((2.0 * hue) < 1.0)
		res = f2;
	else if ((3.0 * hue) < 2.0)
		res = f1 + (f2 - f1) * ((2.0 / 3.0) - hue) * 6.0;
	else
		res = f1;
	return res;
}

inline glm::vec3 hsl2rgb(glm::vec3 hsl) {
	glm::vec3 rgb;
	
	if (hsl.y == 0.0) {
		rgb = glm::vec3(hsl.z); // Luminance
	} else {
		float f2;
		
		if (hsl.z < 0.5)
			f2 = hsl.z * (1.0 + hsl.y);
		else
			f2 = hsl.z + hsl.y - hsl.y * hsl.z;
			
		float f1 = 2.0 * hsl.z - f2;
		
		rgb.r = hue2rgb(f1, f2, hsl.x + (1.0/3.0));
		rgb.g = hue2rgb(f1, f2, hsl.x);
		rgb.b = hue2rgb(f1, f2, hsl.x - (1.0/3.0));
	}   
	return rgb;
}

// https://www.cse.chalmers.se/~uffe/clustered_shading_preprint.pdf
// https://www.researchgate.net/publication/232836241_Tiled_Shading

inline void InitScene(Scene& mainScene) {
	srand(0);

	mainScene.AddComponent<DebugInspector>();
	mainScene.AddComponent<Physics::System>();

#pragma region World

	ShaderProgram* skyProg = ShaderProgram::Build()
	.WithVertexShader(("./res/shaders/skybox.vert"))
	.WithPixelShader(("./res/shaders/skybox.frag"))
	.Link();

	ShaderProgram* sphereProg = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/basic.vert")
	.WithPixelShader("./res/shaders/halo.frag")
	.Link();

	Cubemap* skyCubemap = mainScene.Resources()->Get<Cubemap>(
		"./res/textures/skybox_showcase.hdr",
		Texture::HDRColorBuffer
	);
	skyCubemap->SetWrapModeU(TextureWrap::Clamp);
	skyCubemap->SetWrapModeV(TextureWrap::Clamp);
	skyCubemap->SetWrapModeW(TextureWrap::Clamp);

	Mesh* sphereMesh = ResourceDatabase::Global->Get<Mesh>("./res/models/sphere5m.obj");

	Material* skyMat = new Material(skyProg);
	skyMat->SetValue("skyboxTexture", skyCubemap);

	Material* sphereMat = new Material(sphereProg);
	sphereMat->SetValue("uColor", glm::vec3(0, 0, 1));

	auto floorNode = mainScene.resources.Get<GltfScene>("./res/models/rooms/room_one_lighttest.glb")->Instantiate(&mainScene, nullptr, "floor");
	floorNode->AddObject<Skybox>(skyMat);

	auto monkey = mainScene.resources.Get<GltfScene>("./res/models/rooms/big_monkey.glb")->Instantiate(&mainScene, nullptr, "Monkey");
	monkey->GlobalTransform().Position() = glm::vec3(30, 15, -40);
	
	SceneNode* lightsRoot = mainScene.CreateNode("Lights root");
	lightsRoot->GlobalTransform().Position() = glm::vec3(0, 20, 0);
	lightsRoot->AddObject<Light>(Light::PointLight(glm::vec3(1.0, 0.6, 0.8), 100, 0.5, 0, 0));

	// for (float x = -48; x <= 48; x += 2) {
	// 	for (float y = -48; y <= 48; y += 2) {
	// 		SceneNode* lightNode = mainScene.CreateNode(lightsRoot, std::format("Light {} : {}", x, y));
	// 		lightNode->GlobalTransform().Position() = glm::vec3(x, 0.5, y);

	// 		float h = (double) rand() / RAND_MAX;
	// 		float s = 1;
	// 		float l = 0.5;
	// 		// float h = 312.0f / 360.0f;
	// 		// float s = 0.767f;
	// 		// float l = 0.471f;

	// 		glm::vec3 rgb = hsl2rgb({h, s, l});

	// 		lightNode->AddObject<Light>(Light::PointLight(rgb, 3, 5, 2));
	// 	}
	// }
#pragma endregion

	SceneNode* cameraNode = mainScene.CreateNode("Camera");

	cameraNode->GlobalTransform().Position() = glm::vec3(0, 50, 0);
	cameraNode->GlobalTransform().Rotation() = glm::quatLookAt(glm::vec3(0, 1, 0), glm::vec3(0, 0, 1));
	cameraNode->AddObject<Camera>(Camera::Perspective(90, 1, 1, 100));
	cameraNode->AddObject<Bloom>();
	auto colorGrading = cameraNode->AddObject<ColorGrading>();
	cameraNode->AddObject<Tonemapper>()->SetOperator(Tonemapper::TonemapperOperator::GranTurismo);

	colorGrading->contrast = 1.09;
	colorGrading->chromaticAberrationStrength = 0.1;

	ShaderProgram* dustProgram = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/particles/particles.vert")
	.WithPixelShader("./res/shaders/particles/particles_blend.frag")
	.Link();

	auto dustMaterial = new Material(dustProgram);
	dustMaterial->SetValue("colorTex",
		mainScene.Resources()->Get<Texture2D>("./res/textures/dust.png", Texture2D::ColorTextureRGBA)
	);
	dustMaterial->SetValue("color", glm::vec4(200.0f, 200.0f, 200.0f, 1.0f));

	floorNode->AddObject<ParticleSpawner>(
		mainScene.Resources()->Get<Mesh>("./res/models/star.obj"),
		dustMaterial,
		ParticleSpawnerSettings{
			.maxParticles = 6144,
			.areaExtents = glm::vec3(20.0f, 3.0f, 20.0f),
			.emissionShapeExtents = glm::vec3(20.0f, 3.0f, 20.0f),
			.minVelocity = glm::vec3(-0.08f, -0.05f, -0.08f),
			.maxVelocity = glm::vec3(0.08f, 0.05f, 0.08f),
			.minInitialAngle = 0.0f,
			.maxInitialAngle = 6.28318f,
			.minAngularVelocity = -0.2f,
			.maxAngularVelocity = 0.2f,
			.rotateY = false,
			.enableLifetime = false,
			.minLifetime = 1.0f,
			.maxLifetime = 10000.0f,
			.minScale = 0.02f,
			.maxScale = 0.03f,
			.alphaMode = AlphaMode::Alpha,
			.enableProximityFade = true,
			.proximityFadeMin = 0.2f,
			.proximityFadeMax = 1.5f,
			.enableDistanceFade = true,
			.distanceFadeMin = 9.0f,
			.distanceFadeMax = 12.0f,
			.enableLifetimeFade = true,
			.enableDepthFade = true,
			.depthFadeDistance = 0.3f,
			.billboardMode = BillboardMode::Enabled,
			.wrapAround = true,
			.continuous = false,
			.useColorRamp = false
		}
	);
}
} // namespace LightingTestScene
