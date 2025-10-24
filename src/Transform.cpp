#include <Transform.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Scene.h>

#include <spdlog/spdlog.h>

bool Transform::IsDirty() const {
	return localTransform.IsDirty() || globalTransform.IsDirty();
}

Transform::TransformAccess& Transform::GlobalTransform() {
	return this->globalTransform;
}

Transform::TransformAccess& Transform::LocalTransform() {
	return this->localTransform;
}

Transform::Transform() :
globalTransform(*this),
localTransform(*this),
parent(nullptr) { }

Transform::Transform(const glm::mat4 transformation) :
globalTransform(*this),
localTransform(*this),
parent(nullptr) {
	this->globalTransform = transformation;
}

Transform::Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) :
globalTransform(*this),
localTransform(*this),
parent(nullptr) {
	this->globalTransform.Position() = translation;
	this->globalTransform.Rotation() = rotation;
	this->globalTransform.Scale() = scale;
}

void Transform::ClearDirty() {
	this->globalTransform.dirty = false;
	this->localTransform.dirty = false;
}
 
Transform::TransformAccess::TransformAccess(Transform& source) :
dirty(false),
source(source),
transformation(glm::identity<glm::mat4>()) { }

void Transform::TransformAccess::MarkDirty() {
	this->dirty = true;

	if (this->source.parent) {
		this->source.parent->MarkChildrenDirty();
	}
}

bool Transform::TransformAccess::IsDirty() const {
	return this->dirty;
}

Transform::PositionAccess Transform::TransformAccess::Position() {
	return Transform::PositionAccess(*this);
}
Transform::RotationAccess Transform::TransformAccess::Rotation() {
	return Transform::RotationAccess(*this);
}
Transform::ScaleAccess Transform::TransformAccess::Scale() {
	return Transform::ScaleAccess(*this);
}

glm::vec3 Transform::TransformAccess::Forward() const {
	return glm::column(this->transformation, 2);
}
glm::vec3 Transform::TransformAccess::Backward() const {
	return -glm::column(this->transformation, 2);
}
glm::vec3 Transform::TransformAccess::Up() const {
	return glm::column(this->transformation, 1);
}
glm::vec3 Transform::TransformAccess::Down() const {
	return -glm::column(this->transformation, 1);
}
glm::vec3 Transform::TransformAccess::Right() const {
	return -glm::column(this->transformation, 0);
}
glm::vec3 Transform::TransformAccess::Left() const {
	return glm::column(this->transformation, 0);
}

glm::mat4 Transform::TransformAccess::Value() const {
	return this->transformation;
}

Transform::TransformAccess::operator glm::mat4() const {
	return this->transformation;
}

Transform::TransformAccess& Transform::TransformAccess::operator=(const glm::mat4& transformation) {
	this->transformation = transformation;

	MarkDirty();

	return *this;
}

Transform::PositionAccess::PositionAccess(TransformAccess& source) :
source(source) { }

glm::vec3 Transform::PositionAccess::Value() const {
	return glm::column(this->source.transformation, 3);
}

Transform::PositionAccess::operator glm::vec3() const {
	return Value();
}

Transform::PositionAccess& Transform::PositionAccess::operator=(const glm::vec3& position) {
	this->source.transformation[3] = glm::vec4(position, 1.0f);

	this->source.MarkDirty();

	return *this;
}

Transform::PositionAccess& Transform::PositionAccess::operator+=(const glm::vec3& position) {
	this->source.transformation[3] += glm::vec4(position, 0.0f);

	this->source.MarkDirty();

	return *this;
}

Transform::PositionAccess& Transform::PositionAccess::operator-=(const glm::vec3& position) {
	this->source.transformation[3] += glm::vec4(position, 0.0f);

	this->source.MarkDirty();

	return *this;
}

Transform::RotationAccess::RotationAccess(TransformAccess& source) :
source(source) { }

glm::quat Transform::RotationAccess::Value() const {
	return glm::quat(this->source.transformation);
}

Transform::RotationAccess::operator glm::quat() const {
	return Value();
}

Transform::RotationAccess& Transform::RotationAccess::operator=(const glm::quat& rotation) {
	glm::vec3 scale = this->source.Scale();

	glm::mat3 rotationMatrix = glm::mat3_cast(rotation);
	
	rotationMatrix[0][0] *= scale.x;
	rotationMatrix[1][1] *= scale.y;
	rotationMatrix[2][2] *= scale.z;

	this->source.transformation[0][0] = rotationMatrix[0][0];
	this->source.transformation[0][1] = rotationMatrix[0][1];
	this->source.transformation[0][2] = rotationMatrix[0][2];
	this->source.transformation[1][0] = rotationMatrix[1][0];
	this->source.transformation[1][1] = rotationMatrix[1][1];
	this->source.transformation[1][2] = rotationMatrix[1][2];
	this->source.transformation[2][0] = rotationMatrix[2][0];
	this->source.transformation[2][1] = rotationMatrix[2][1];
	this->source.transformation[2][2] = rotationMatrix[2][2];

	this->source.MarkDirty();

	return *this;
}

Transform::RotationAccess& Transform::RotationAccess::operator*=(const glm::quat& rotation) {
	*this = Value() * rotation;

	this->source.MarkDirty();

	return *this;
}

Transform::ScaleAccess::ScaleAccess(TransformAccess& source) :
source(source) { }

glm::vec3 Transform::ScaleAccess::Value() const {
	return glm::vec3(
		glm::length(glm::column(this->source.transformation, 0)),
		glm::length(glm::column(this->source.transformation, 1)),
		glm::length(glm::column(this->source.transformation, 2))
	);
}

Transform::ScaleAccess::operator glm::vec3() const {
	return Value();
}

Transform::ScaleAccess& Transform::ScaleAccess::operator=(const glm::vec3& scale) {
	glm::vec3 current_scale = Value();

	this->source.transformation[0][0] /= current_scale.x;
	this->source.transformation[0][0] *= scale.x;
	this->source.transformation[1][1] /= current_scale.y;
	this->source.transformation[1][1] *= scale.y;
	this->source.transformation[2][2] /= current_scale.z;
	this->source.transformation[2][2] *= scale.z;

	this->source.MarkDirty();

	return *this;
}

Transform::ScaleAccess& Transform::ScaleAccess::operator*=(const glm::vec3& scale) {
	glm::vec3 current_scale = Value();

	this->source.transformation[0][0] *= scale.x;
	this->source.transformation[1][1] *= scale.y;
	this->source.transformation[2][2] *= scale.z;

	this->source.MarkDirty();

	return *this;
}