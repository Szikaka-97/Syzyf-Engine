#include "game_scripts/AttackEffects/EffectBase.h"
#include "game_scripts/enemies/EnemyBullet.h"
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>
#include "game_scripts/AttackEffects/EffectsManager.h"
#include "game_scripts/FireParticles.h"
#include "Light.h"
#include "fog/FogVolume.h"
#include "game_scripts/FireParticles.h"
#include "../../game/include/scenes/examples/particles_and_scatter.h"
#include "glm/gtc/random.hpp"


//namespace ExampleFogVolume {
//class Tornado;}namespace ExampleParticlesAndScatter {
//class Tornado;}
void FlameFlicker::OnEnable() {
    auto* tweenSystem = this->GetScene()->GetComponent<TweenSystem>();
    if (!tweenSystem || !fireLight) return;

    float targetIntensity = baseIntensity + glm::linearRand(-1.5f, 1.5f);
    float duration = glm::linearRand(0.06f, 0.14f);

    TweenConfig config;
    config.initialValue = this->fireLight->GetIntensity();
    config.targetValue = targetIntensity;
    config.duration = duration;
    config.easingFunction = Easing::inOutSine;

    this->flickerTween = std::move(
        tweenSystem->CreateTween(config)
            .Bind([this](float newValue) {
                if (fireLight) this->fireLight->SetIntensity(newValue);
            })
            .OnComplete([this]() {
                this->OnEnable();
            })
    );
}

void EffectFire::OnInit() {
    Scene*     mainScene    = GetScene();
    SceneNode* fireRootNode = GetNode();
    fireRootNode->GlobalTransform().Scale() = glm::vec3(1.0f, 1.0f, 1.0f);

    ShaderProgram* fireProgram =
        ShaderProgram::Build()
            .WithVertexShader("./res/shaders/particles/particles.vert")
            .WithPixelShader("./res/shaders/particles/particles_blend.frag")
            .Link();

    Texture2D* dustTex = mainScene->Resources()->Get<Texture2D>(
        "./res/textures/smoke_08.png", Texture2D::ColorTextureRGBA);

    // TextureParams rampParams = {
    //     .channels   = TextureChannels::RGB,
    //     .colorSpace = TextureColor::Linear,
    //     .format     = TextureFormat::Ubyte,
    //     .wrapU      = TextureWrap::Clamp,
    //     .wrapV      = TextureWrap::Clamp,
    //     .minFilter  = TextureFilter::Linear,
    //     .magFilter  = TextureFilter::Linear,
    // };
    // Texture2D* gradientTex = mainScene->Resources()->Get<Texture2D>(
    //     "./res/textures/color_grading_lut.png", rampParams);

    auto* fireMaterial = new Material(fireProgram);
    fireMaterial->SetValue("colorTex",  dustTex);
    //fireMaterial->SetValue("colorRamp", gradientTex);
    // Biały tint — kolor w całości pochodzi z colorRamp
    fireMaterial->SetValue("color", glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));

    ParticleSpawnerSettings fireSettings = {
        .maxParticles         = 250,
        .areaExtents          = glm::vec3(2.0f, 5.0f, 2.0f),
        .emissionShapeExtents = glm::vec3(1.0f, 0.05f, 1.0f),

        .minVelocity = glm::vec3(-0.20f, 1.2f, -0.20f),
        .maxVelocity = glm::vec3( 0.20f, 2.2f,  0.20f),

        .minInitialAngle    = 0.0f,
        .maxInitialAngle    = 6.28318f,
        .minAngularVelocity = -1.0f,
        .maxAngularVelocity =  1.0f,
        .rotateY            = false,

        .enableLifetime = true,
        .minLifetime    = 0.5f,
        .maxLifetime    = 1.0f,

        .minScale = 0.25f,
        .maxScale = 0.50f,

        .color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),

        .alphaMode = AlphaMode::Alpha,

        .enableLifetimeFade = true,
        .lifetimeFadeIn     = {0.0f, 0.30f},
        .lifetimeFadeOut    = {0.65f, 1.0f},

        .enableDepthFade   = true,
        .depthFadeDistance = 0.15f,

        .billboardMode = BillboardMode::Enabled,
        .wrapAround    = false,
        .continuous    = true,
        .useColorRamp  = false,

    };

    fireRootNode->AddObject<ParticleSpawner>(
        mainScene->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj"),
        fireMaterial,
        fireSettings);

    SceneNode* lightNode = mainScene->CreateNode(fireRootNode, "Fire Light Asset");
    lightNode->LocalTransform().Position() = {0.0f, 0.4f, 0.0f};

    auto* fireLight = lightNode->AddObject<Light>(
        Light::PointLight({1.0f, 1.f, 1.f}, 4.0f, 1.0f));

    auto* flickerScript = lightNode->AddObject<FlameFlicker>();
    flickerScript->Init(fireLight);
}

void EffectFire::OnApplySpecials() {
    if (special1) damage *= modifier;
    if (special2) dotRemainingTime *= static_cast<float>(modifier);
    spdlog::info("FireEffect: radius={:.1f}, damage={:.0f}, burnDur={:.1f}s",
                 radius, damage, dotRemainingTime);
}
void EffectFire::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->ApplyBurn(damage, dotRemainingTime, timeInterval);
}

void EffectPetrify::OnInit() {

    if (myNode) myNode->GlobalTransform().Position() = glm::vec3(GlobalTransform().Position().x,0.4f,GlobalTransform().Position().z);
    SceneNode* effectModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/petrify.glb")
->Instantiate(GetScene(), GetNode(), "petrify effect");
    effectModel->GlobalTransform().Scale()=glm::vec3(1.f,1.5f,1.5f);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);

    effectModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    for (auto& anim : effectModel->GetObject<AnimationComponent>()->animations) {
        anim.speed = 4.0;
        effectModel->GetObject<AnimationComponent>()->Play(anim.data.name);
    }
}

void EffectPetrify::OnApplySpecials() {
    if (special1) petrifyRemainingTime *= static_cast<float>(modifier);
    if (special2) radius *= static_cast<float>(modifier);
    spdlog::info("PetrifyEffect: radius={:.1f}, freezeDur={:.1f}s",
                 radius, petrifyRemainingTime);
}
void EffectPetrify::OnApplyToEnemy(EnemyBase* enemy) {
    if (enemy->IsPetrified()) return;
    float slowFactor = (ingredientCount == 1) ? 0.5f : 0.0f;
    enemy->ApplyPetrify(slowFactor, petrifyRemainingTime);
}

void EffectTornado::OnInit(){
    if (myNode) myNode->GlobalTransform().Position() = glm::vec3(GlobalTransform().Position().x,0.4f,GlobalTransform().Position().z);
    SceneNode* effectModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/tornado.glb")
->Instantiate(GetScene(), GetNode(), "tornado effect");
    effectModel->GlobalTransform().Scale()=glm::vec3(2.f,2.5f,2.f);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);

    effectModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    auto* animComp = effectModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->Play("Animation.001");
    }
}
void EffectTornado::OnApplySpecials() {
    if (special1) radius               *= static_cast<float>(modifier);
    if (special2) tornadoRemainingTime *= static_cast<float>(modifier);
    spdlog::info("TornadoEffect: radius={:.1f}, duration={:.1f}s", radius, tornadoRemainingTime);
}
void EffectTornado::OnUpdate() {
    if (myNode) {
        glm::quat delta = glm::angleAxis(
            glm::radians(rotationSpeed * Time::Delta()),
            glm::vec3(0.0f, 1.0f, 0.0f));
        myNode->GlobalTransform().Rotation() =
            delta * glm::quat(myNode->GlobalTransform().Rotation());
    }
    SpinEnemies();
    m_DamageTimer += Time::Delta();
    if (m_DamageTimer >= damageInterval) {
        m_DamageTimer = 0.0f;
        for (auto* enemy : ScanEnemiesInRadius())
            enemy->TakeDamage(static_cast<int>(damage));
    }
    ScanAndHandleBullets();
}

void EffectTornado::SpinEnemies() {
    glm::vec3 center = GetPosition();
    for (auto* enemy : ScanEnemiesInRadius()) {
        enemy->ApplyOrbitalVelocity(center, rotationSpeed);
    }
}

void EffectTornado::ScanAndHandleBullets() {
    glm::vec3 pos = GetPosition();
    for (auto* bullet : GetScene()->FindObjectsOfType<EnemyBullet>()) {
        if (!bullet->myNode) continue;
        glm::vec3 bpos = bullet->myNode->GlobalTransform().Position();
        if (glm::distance(pos, bpos) > radius) continue;
        if (ingredientCount == 1)
            GetScene()->QueueDelete(bullet->myNode);
        else
            bullet->BulletInTornadoAction(myNode, radius, rotationSpeed);
    }
}

void EffectConfuse::OnInit() {
    if (myNode) myNode->GlobalTransform().Position() = glm::vec3(GlobalTransform().Position().x,0.4f,GlobalTransform().Position().z);
    SceneNode* effectModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/confuse.glb")
->Instantiate(GetScene(), GetNode(), "confuse effect");
    effectModel->GlobalTransform().Scale()=glm::vec3(1.f,1.5f,1.5f);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);

    effectModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    auto* animComp = effectModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->Play("Animation");
    }
}

void EffectConfuse::OnApplySpecials() {
    if (special1) confuseRemainingTime *= static_cast<float>(modifier);
    if (special2) damage *= modifier;
    spdlog::info("ConfuseEffect: radius={:.1f}, confuseDur={:.1f}s", radius, confuseRemainingTime);
}
void EffectConfuse::OnApplyToEnemy(EnemyBase* enemy) {
    bool attackPrecisely = (ingredientCount == 2);
    enemy->ApplyConfuse(confuseRemainingTime, attackPrecisely);
}

void EffectExplosion::OnInit() {


    SceneNode* explosionModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/explode.glb")
->Instantiate(GetScene(), GetNode(), "explosion effect");
    explosionModel->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    explosionModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    this->radius = GetRange();

    for (auto& anim : explosionModel->GetObject<AnimationComponent>()->animations) {
        anim.speed = 2.0;
        explosionModel->GetObject<AnimationComponent>()->Play(anim.data.name);
    }

    explosionModel->FindNode("SphereScaler")->LocalTransform().Scale() = glm::vec3(this->radius);
}

void EffectExplosion::OnApplySpecials() {
    speed = 1.0f;

    spdlog::info("EffectExplosion: range={:.1f}, damage={:.1f}, duration={:.1f}s",
                 GetRange(), GetDamage(), explosionDuration);
}

void EffectExplosion::OnUpdate() {

}

void EffectExplosion::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));
}