#pragma once

#include "GameObject.h"
#include "Material.h"
#include "Mesh.h"
#include "ParticleSpawner.h"
#include "Shader.h"

class FireParticles : public GameObject {
public:
    FireParticles() {
        // Main particles
        ShaderProgram* sprayProgram =
            ShaderProgram::Build()
                .WithVertexShader("./res/shaders/particles/particles.vert")
                .WithPixelShader("./res/shaders/particles/particles_blend.frag")
                .Link();

        auto sprayMaterial = new Material(sprayProgram);
        sprayMaterial->SetValue("colorTex",
                                GetScene()->Resources()->Get<Texture2D>(
                                    "./res/textures/fire_particles/T_fire01.png",
                                    Texture2D::ColorTextureRGBA));
        sprayMaterial->SetValue("color",
                                glm::vec4(1.0f * 1.2f, 166.0f / 255.0f * 1.2f,
                                          127.0f / 255.0f * 1.2f, 1.0f));
        sprayMaterial->SetValue("colorRamp",
                                GetScene()->Resources()->Get<Texture2D>(
                                    "./res/textures/fire_particles/color_ramp.png",
                                    Texture2D::ColorTextureRGB));

        GetNode()->AddObject<ParticleSpawner>(
            GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj"),
            sprayMaterial,
            ParticleSpawnerSettings{
                .maxParticles = 64,
                .areaExtents = glm::vec3(0.5f, 3.0f, 0.5f),
                .emissionShapeExtents = glm::vec3(0.1f, 0.1f, 0.1f),
                .minVelocity = glm::vec3(-0.1f, 0.5f, -0.1f),
                .maxVelocity = glm::vec3(0.1f, 0.5f, 0.1f),
                .rotateY = false,
                .enableLifetime = true,
                .minLifetime = 1.0f,
                .maxLifetime = 2.0f,
                .minScale = 0.1f,
                .maxScale = 0.25f,

                .scaleCurveTexture = GetScene()->Resources()->Get<Texture2D>(
                    "./res/textures/fire_particles/scale_ramp.png",
                    Texture2D::TechnicalMapXYZ),

                .color = glm::vec4(1.0f, 61.0f / 255.0f, 0.0f, 1.0f),
                .colorIntensity = 10.0f,

                .alphaMode = AlphaMode::Alpha,
                .enableLifetimeFade = true,
                .enableDepthFade = true,
                .depthFadeDistance = 0.2f,
                .billboardMode = BillboardMode::Enabled,
                .wrapAround = false,
                .continuous = true,
                .useColorRamp = true,
                .spawnLights = true,
                .lightStrength = 0.1
            });

        // Small sparks
        ShaderProgram* sparksProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/particles/particles.vert")
            .WithPixelShader("./res/shaders/particles/particles_blend.frag")
            .Link();

        auto sparksMaterial = new Material(sparksProgram);
        sparksMaterial->SetValue("colorTex",
                                 GetScene()->Resources()->Get<Texture2D>(
                                     "./res/textures/dust.png",
                                     Texture2D::ColorTextureRGBA));
        sparksMaterial->SetValue("colorRamp",
                                GetScene()->Resources()->Get<Texture2D>(
                                    "./res/textures/fire_particles/color_ramp.png",
                                    Texture2D::ColorTextureRGB));

        GetNode()->AddObject<ParticleSpawner>(
            GetScene()->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj"),
            sparksMaterial,
            ParticleSpawnerSettings{
                .maxParticles = 32,
                .areaExtents = glm::vec3(1.0f, 3.5f, 1.0f),
                .emissionShapeExtents = glm::vec3(0.25f, 0.25f, 0.25f),
                .minVelocity = glm::vec3(-0.3f, 0.8f, -0.3f),
                .maxVelocity = glm::vec3(0.3f, 1.5f, 0.3f),
                .rotateY = false,
                .enableLifetime = true,
                .minLifetime = 0.5f,
                .maxLifetime = 1.2f,
                .minScale = 0.001f,
                .maxScale = 0.005f,

                // .scaleCurveTexture = GetScene()->Resources()->Get<Texture2D>(
                //     "./res/textures/fire_particles/scale_ramp.png",
                //     Texture2D::TechnicalMapXYZ),

                .color = glm::vec4(1.0f, 180.0f / 255.0f, 50.0f / 255.0f, 1.0f),
                .colorIntensity = 1000.0f,

                .alphaMode = AlphaMode::Alpha, 
                .enableLifetimeFade = true,
                .enableDepthFade = true,
                .depthFadeDistance = 0.2f,
                .billboardMode = BillboardMode::Enabled,
                .wrapAround = false,
                .continuous = true,
                .useColorRamp = true});
    }
};
