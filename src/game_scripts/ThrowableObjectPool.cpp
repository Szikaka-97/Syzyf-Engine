#include <game_scripts/ThrowableObjectPool.h>

#include <MeshRenderer.h>
#include <Mesh.h>
#include <Material.h>
#include <Shader.h>
#include <game_scripts/ThrowableObject.h>
#include <physics/Body.h>
#include <physics/Helpers.h>
#include "physics/System.h"

ThrowableObjectPool::ThrowableObjectPool(Scene *scene):
SceneComponent(scene),
capacity(20) {
	this->allInstances.reserve(this->capacity);

	this->bottlesPoolRoot = GetScene()->CreateNode("Bottles Pool");

	this->bottleMesh = GetScene()->Resources()->Get<Mesh>("./res/models/butelka.glb");

	auto bottleShader = ShaderProgram::Build()
	.WithVertexShader("./res/shaders/lit.vert")
	.WithPixelShader("./res/shaders/lambert color.frag")
	.Link();

	this->bottleMaterial = new Material(bottleShader);
	this->bottleMaterial->SetValue("uColor", glm::vec3(1, 0, 0));
}

SceneNode* ThrowableObjectPool::RequestThrowableObject() {
	if (this->allInstances.size() < this->capacity) {
		SceneNode* newBottle = GetScene()->CreateNode(this->bottlesPoolRoot, std::format("Bottle {}", this->allInstances.size()));

		// newBottle->AddObject<T>();

		newBottle->AddObject<Physics::Body>(JPH::BodyCreationSettings(Physics::SphereShape(0.1f), JPH::Vec3::sZero(), JPH::Quat::sIdentity(), JPH::EMotionType::Dynamic, Physics::Layers::MOVING))->SetCollisionLayerAndMask({0}, 0);;
		newBottle->AddObject<MeshRenderer>(this->bottleMesh, this->bottleMaterial);

		this->allInstances.push_back({newBottle, (int) this->allInstances.size()});

		std::sort(this->allInstances.begin(), this->allInstances.end());

		return newBottle;
	}
	else {
		auto& bottle = this->allInstances.front();

		bottle.id = this->capacity + bottle.id;

		SceneNode* bottleNode = bottle.bottle;

		std::sort(this->allInstances.begin(), this->allInstances.end());

		return bottleNode;
	}
}

void ThrowableObjectPool::ReturnBottleToPool(ThrowableObject* bottle) {
	bottle->GetNode()->SetEnabled(false);
	bottle->GetNode()->SetParent(this->bottlesPoolRoot);
}