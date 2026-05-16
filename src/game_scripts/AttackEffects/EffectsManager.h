#pragma once
#include "EffectBase.h"

class EffectFire : public EffectBase {
public:
    float dotRemainingTime = 5.0f;   
    float damage           = 25.0f;  
    float timeInterval     = 1.0f;  
    EffectFire();
    EffectBase* Clone() const { return new EffectFire(*this); }  
protected:
    void OnApplySpecials() override;
    void OnApplyToEnemy(EnemyBase* enemy) override;
};


class EffectPetrify : public EffectBase {
public:
    float petrifyRemainingTime = 5.0f;
    EffectPetrify();
    EffectBase* Clone() const { return new EffectPetrify(*this); }  

protected:
    void OnApplySpecials() override;
    void OnApplyToEnemy(EnemyBase* enemy) override;
};

class EffectTornado : public EffectBase {
public:
    float tornadoRemainingTime = 5.0f;
    float rotationSpeed        = 90.0f;  
    float damage               = 25.0f;
    float damageInterval       = 1.0f;   
    EffectTornado();
    EffectBase* Clone() const { return new EffectTornado(*this); }  

protected:
    void  OnApplySpecials() override;
    void  OnApplyToEnemy(EnemyBase* enemy) override {}  
    void  OnUpdate() override;
    float GetMaxLifetime() const override { return tornadoRemainingTime; }

private:
    float m_DamageTimer = 0.0f;   
    void  ScanAndHandleBullets();
};


class EffectConfuse : public EffectBase {
public:
    float confuseRemainingTime = 5.0f;
    int   damage               = 25; 
    EffectConfuse();
    EffectBase* Clone() const { return new EffectConfuse(*this); }
protected:
    void OnApplySpecials() override;
    void OnApplyToEnemy(EnemyBase* enemy) override;
};

class EffectExplosion : public EffectBase {
public:
    float strength  = 0.5f;
    float maxRange  = 10.0f;
    float maxDamage = 50.0f;
    EffectExplosion();
    EffectBase* Clone() const { return new EffectExplosion(*this); }
    float GetRange()  const { return strength * maxRange;  }
    float GetDamage() const { return strength * maxDamage; }
 
protected:
    void  OnApplySpecials() override;
    void  OnApplyToEnemy(EnemyBase* enemy) override;
    void  OnUpdate() override;
    float GetMaxLifetime() const override { return GetRange(); }
};