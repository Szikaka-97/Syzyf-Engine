#pragma once
#include <GameObject.h>

class EnemyBase; 

class EnemyBullet : public GameObject {
public:
    EnemyBase* owner = nullptr;  /

    void BulletInTornadoAction(SceneNode* tornadoNode,
                               float      orbitRadius,
                               float      rotationSpeed);

    void Update();

private:
    // Filled in by BulletInTornadoAction
    bool       m_Orbiting      = false;
    SceneNode* m_OrbitCenter   = nullptr;
    float      m_OrbitRadius   = 0.0f;
    float      m_OrbitAngle    = 0.0f;   
    float      m_OrbitSpeed    = 0.0f;   
};