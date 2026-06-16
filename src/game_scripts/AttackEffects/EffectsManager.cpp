#include "game_scripts/AttackEffects/EffectBase.h"
#include "game_scripts/enemies/EnemyBullet.h"
#include <TimeSystem.h>
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <spdlog/spdlog.h>
#include "game_scripts/AttackEffects/EffectsManager.h"

#include "Light.h"
#include "game_scripts/FireParticles.h"

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

    // Tekstura kształtu cząstki (soft circle, alpha).
    Texture2D* dustTex = mainScene->Resources()->Get<Texture2D>(
        "./res/textures/smoke_08.png", Texture2D::ColorTextureRGBA);

    // Gradient kolor: t=0 (nowa cząstka, przy ziemi) fiolet
    //                 t=0.5 pomarańcz
    //                 t=1 (stara, przy czubku) żółty
    // Plik do skopiowania do ./res/textures/fire_gradient.png
    TextureParams rampParams = {
        .channels   = TextureChannels::RGB,
        .colorSpace = TextureColor::Linear,
        .format     = TextureFormat::Ubyte,
        .wrapU      = TextureWrap::Clamp,
        .wrapV      = TextureWrap::Clamp,
        .minFilter  = TextureFilter::Linear,
        .magFilter  = TextureFilter::Linear,
    };
    Texture2D* gradientTex = mainScene->Resources()->Get<Texture2D>(
        "./res/textures/fire_gradient.png", rampParams);

    auto* fireMaterial = new Material(fireProgram);
    fireMaterial->SetValue("colorTex",  dustTex);
    fireMaterial->SetValue("colorRamp", gradientTex);
    // Biały tint — kolor w całości pochodzi z colorRamp
    fireMaterial->SetValue("color", glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));

    // ------------------------------------------------------------------
    // areaExtents.y = 5.0 — żadna cząstka nie przekroczy granicy obszaru
    // w trakcie swojego życia (max: 2.2 m/s × 1.0 s = 2.2 u << 5.0 u),
    // co eliminuje reset pozycji cząstki do (0,0,0) po wyjściu z obszaru.
    //
    // lifetimeFadeIn = {0.0, 0.3} — cząstki inicjalizowane w puli zaczynają
    // od alpha=0 i stopniowo wchodzą w widoczność; maskuje artefakty
    // pierwszej klatki, gdy układ particles jeszcze nie wylosował pozycji.
    // ------------------------------------------------------------------
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

        .alphaMode = AlphaMode::Alpha,

        .enableLifetimeFade = true,
        .lifetimeFadeIn     = {0.0f, 0.30f},
        .lifetimeFadeOut    = {0.65f, 1.0f},

        .enableDepthFade   = true,
        .depthFadeDistance = 0.15f,

        .billboardMode = BillboardMode::Enabled,
        .wrapAround    = false,
        .continuous    = true,
        .useColorRamp  = true,   // ← silnik próbkuje colorRamp przez czas życia
    };

    fireRootNode->AddObject<ParticleSpawner>(
        mainScene->Resources()->Get<Mesh>("./res/models/fullscreenquad.obj"),
        fireMaterial,
        fireSettings);

    // Światło + migotanie
    SceneNode* lightNode = mainScene->CreateNode(fireRootNode, "Fire Light Asset");
    lightNode->LocalTransform().Position() = {0.0f, 0.4f, 0.0f};

    auto* fireLight = lightNode->AddObject<Light>(
        Light::PointLight({1.0f, 0.45f, 0.08f}, 4.0f, 1.0f));

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
    ->Get<GltfScene>("./res/models/effects/petrify1.glb")
->Instantiate(GetScene(), GetNode(), "petrify effect");
    effectModel->GlobalTransform().Scale()=glm::vec3(1.f,1.5f,1.5f);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);

    effectModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    auto* animComp = effectModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->Play("Cylinder.008Action");
        animComp->Play("Cylinder.009Action");
        animComp->Play("Cylinder.010Action");
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
    SceneNode* effectModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/tornado1.glb")
->Instantiate(GetScene(), GetNode(), "tornado effect");
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.f,1.5f,1.5f);
    effectModel->LocalTransform().Position() = glm::vec3(0, 0, 0);

    auto* animComp = effectModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->Play("Cylinder.012Action");
    }
}


void EffectTornado::OnApplySpecials() {
    if (special1) radius               *= static_cast<float>(modifier);
    if (special2) tornadoRemainingTime *= static_cast<float>(modifier);
    spdlog::info("TornadoEffect: radius={:.1f}, duration={:.1f}s", radius, tornadoRemainingTime);
}
void EffectTornado::OnUpdate() {
    /*if (myNode) {
        glm::quat delta = glm::angleAxis(
            glm::radians(rotationSpeed * Time::Delta()),
            glm::vec3(0.0f, 1.0f, 0.0f));
        myNode->GlobalTransform().Rotation() =
            delta * glm::quat(myNode->GlobalTransform().Rotation());
    }*/
    m_DamageTimer += Time::Delta();
    if (m_DamageTimer >= damageInterval) {
        m_DamageTimer = 0.0f;
        for (auto* enemy : ScanEnemiesInRadius())
            enemy->TakeDamage(static_cast<int>(damage));
    }
    ScanAndHandleBullets();
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
    SceneNode* effectModel =
            ResourceDatabase::Global
    ->Get<GltfScene>("./res/models/effects/confuse1.glb")
->Instantiate(GetScene(), GetNode(), "confuse effect");
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.5f,1.5f,1.5f);

    effectModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    auto* animComp = effectModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->Play("Spiral.002Action");
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
    ->Get<GltfScene>("./res/models/effects/explode1.glb")
->Instantiate(GetScene(), GetNode(), "explosion effect");
    explosionModel->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);
    explosionModel->LocalTransform().Position() = glm::vec3(0, 0, 0);
    GetNode()->GlobalTransform().Scale()=glm::vec3(1.0f,1.0f,1.0f);

    auto* animComp = explosionModel->GetObjectInChildren<AnimationComponent>();
    if (animComp) {
        animComp->Play("*Action");
        animComp->Play("CylinderAction");
        animComp->Play("TorusAction");
        animComp->Play("SphereAction");
    }
}

void EffectExplosion::OnApplySpecials() {
    speed = 1.0f;

    spdlog::info("EffectExplosion: range={:.1f}, damage={:.1f}, duration={:.1f}s",
                 GetRange(), GetDamage(), explosionDuration);
}

void EffectExplosion::OnApplyToEnemy(EnemyBase* enemy) {
    enemy->TakeDamage(static_cast<int>(GetDamage()));
}