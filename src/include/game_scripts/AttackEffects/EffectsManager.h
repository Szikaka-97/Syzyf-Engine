#pragma once
#include "EffectBase.h"
#include "Light.h"
#include "TweenSystem.h"
#include "game_scripts/FireParticles.h"
#include "glm/gtc/random.hpp"

#include <glm/gtc/constants.hpp>
#include <random>
#include <vector>

class FlameFlicker : public GameObject {
private:
    Light* fireLight = nullptr;
    TweenHandle flickerTween;
    float baseIntensity = 4.0f;

public:
    void Init(Light* light) {
        this->fireLight = light;
    }

    void OnEnable() ;
};

class EffectFire : public EffectBase {
public:
    serialized float dotRemainingTime = 5.0f;
    serialized float damage           = 25.0f;
    serialized float timeInterval     = 1.0f;

    serialized float strength         = 0.5f;
    serialized float maxRange         = 10.0f;
    serialized float maxDamage        = 0.0f;

    EffectFire() = default;
protected:

    void  OnInit()          override;
    void OnApplySpecials() override;
    void OnApplyToEnemy(EnemyBase* enemy) override;
};

class EffectPetrify : public EffectBase {
public:
    serialized float petrifyRemainingTime = 5.0f;
    EffectPetrify() = default;


    serialized float strength         = 0.5f;
    serialized float maxRange         = 10.0f;
    serialized float maxDamage        = 0.0f;
protected:
    void  OnInit()          override;
    void OnApplySpecials() override;
    void OnApplyToEnemy(EnemyBase* enemy) override;
};

class EffectTornado : public EffectBase {
public:
    serialized float tornadoRemainingTime = 5.0f;
    serialized float rotationSpeed        = 90.0f;
    serialized float damage               = 25.0f;
    serialized float damageInterval       = 1.0f;

    serialized float strength         = 0.5f;
    serialized float maxRange         = 10.0f;
    serialized float maxDamage        = 0.0f;

    EffectTornado() = default;
protected:

    void  OnInit()          override;
    void  OnApplySpecials() override;
    void  OnApplyToEnemy(EnemyBase* /*enemy*/) override {}
    void  OnUpdate() override;
    float GetMaxLifetime() const override { return tornadoRemainingTime; }
private:
    float m_DamageTimer = 0.0f;
    void  ScanAndHandleBullets();
};

class EffectConfuse : public EffectBase {
public:
    serialized float confuseRemainingTime = 50.0f;
    serialized int   damage               = 25;
    EffectConfuse() = default;


    serialized float strength         = 0.5f;
    serialized float maxRange         = 10.0f;
    serialized float maxDamage        = 50.0f;
protected:

    void  OnInit()          override;
    void OnApplySpecials() override;
    void OnApplyToEnemy(EnemyBase* enemy) override;
};

class EffectExplosion : public EffectBase {
public:
    serialized float strength         = 0.5f;
    serialized float maxRange         = 10.0f;
    serialized float maxDamage        = 50.0f;
    serialized float explosionDuration = 2.5f;   // seconds

    EffectExplosion() = default;

    float GetRange()  const { return strength * maxRange;  }
    float GetDamage() const { return strength * maxDamage; }

protected:
    void  OnInit()          override;
    void  OnApplySpecials() override;
    void  OnApplyToEnemy(EnemyBase* enemy) override;
    float GetMaxLifetime()  const override { return explosionDuration; }
};