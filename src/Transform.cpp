#include <Transform.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_access.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Scene.h>

#include <spdlog/spdlog.h>

bool SceneTransform::IsDirty() const {
	return localTransform.IsDirty() || globalTransform.IsDirty();
}

SceneTransform::TransformAccess& SceneTransform::GlobalTransform() {
	return this->globalTransform;
}

SceneTransform::TransformAccess& SceneTransform::LocalTransform() {
	return this->localTransform;
}

SceneTransform::SceneTransform() :
globalTransform(*this),
localTransform(*this),
parent(nullptr) { }

SceneTransform::SceneTransform(const glm::mat4& transformation) :
globalTransform(*this),
localTransform(*this),
parent(nullptr) {
	this->globalTransform = transformation;
}

SceneTransform::SceneTransform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale) :
globalTransform(*this),
localTransform(*this),
parent(nullptr) {
	this->globalTransform.Position() = translation;
	this->globalTransform.Rotation() = rotation;
	this->globalTransform.Scale() = scale;
}

void SceneTransform::ClearDirty() {
	this->globalTransform.dirty = false;
	this->localTransform.dirty = false;
}
 
SceneTransform::TransformAccess::TransformAccess(SceneTransform& source) :
dirty(false),
source(source),
transformation(glm::identity<glm::mat4>()) { }

void SceneTransform::TransformAccess::MarkDirty() {
	this->dirty = true;

	if (this->source.parent) {
		this->source.parent->MarkChildrenDirty();
	}
}

bool SceneTransform::TransformAccess::IsDirty() const {
	return this->dirty;
}

SceneTransform::PositionAccess SceneTransform::TransformAccess::Position() {
	return SceneTransform::PositionAccess(*this);
}
SceneTransform::RotationAccess SceneTransform::TransformAccess::Rotation() {
	return SceneTransform::RotationAccess(*this);
}
SceneTransform::ScaleAccess SceneTransform::TransformAccess::Scale() {
	return SceneTransform::ScaleAccess(*this);
}

glm::vec3 SceneTransform::TransformAccess::Forward() const {
	return glm::column(this->transformation, 2);
}
glm::vec3 SceneTransform::TransformAccess::Backward() const {
	return -glm::column(this->transformation, 2);
}
glm::vec3 SceneTransform::TransformAccess::Up() const {
	return glm::column(this->transformation, 1);
}
glm::vec3 SceneTransform::TransformAccess::Down() const {
	return -glm::column(this->transformation, 1);
}
glm::vec3 SceneTransform::TransformAccess::Right() const {
	return glm::column(this->transformation, 0);
}
glm::vec3 SceneTransform::TransformAccess::Left() const {
	return -glm::column(this->transformation, 0);
}

glm::mat4 SceneTransform::TransformAccess::Value() const {
	return this->transformation;
}

SceneTransform::TransformAccess::operator glm::mat4() const {
	return this->transformation;
}

SceneTransform::TransformAccess& SceneTransform::TransformAccess::operator=(const glm::mat4& transformation) {
	this->transformation = transformation;

	MarkDirty();

	return *this;
}

SceneTransform::PositionAccess::PositionAccess(TransformAccess& source) :
source(source),
value(glm::column(source.transformation, 3)) { }

SceneTransform::PositionAccess::~PositionAccess() {
	glm::vec3 oldValue = glm::column(source.transformation, 3);

	if (oldValue != this->value) {
		this->source.transformation[3] = glm::vec4(this->value, 1.0f);
		this->source.MarkDirty();
	}
}

glm::vec3 SceneTransform::PositionAccess::Value() const {
	return this->value;
}

SceneTransform::PositionAccess::operator glm::vec3() const {
	return Value();
}

SceneTransform::PositionAccess::operator glm::vec2() const {
	return (glm::vec2) Value();
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::operator=(const glm::vec3& position) {
	this->value = position;

	return *this;
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::operator+=(const glm::vec3& position) {
	this->value += position;

	return *this;
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::operator-=(const glm::vec3& position) {
	this->value -= position;

	return *this;
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::operator=(const glm::vec2& position) {
	this->value = glm::vec3(position, 0.0);

	return *this;
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::operator+=(const glm::vec2& position) {
	this->value += glm::vec3(position, 0.0);

	return *this;
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::operator-=(const glm::vec2& position) {
	this->value -= glm::vec3(position, 0.0);

	return *this;
}

SceneTransform::PositionAccess& SceneTransform::PositionAccess::SetX(float x) {
	this->x = x;

	return *this;
}
SceneTransform::PositionAccess& SceneTransform::PositionAccess::SetY(float y) {
	this->y = y;

	return *this;
}
SceneTransform::PositionAccess& SceneTransform::PositionAccess::SetZ(float z) {
	this->z = z;

	return *this;
}

glm::vec3 SceneTransform::PositionAccess::WithX(float x) const {
	return glm::vec3(x, this->y, this->z);
}
glm::vec3 SceneTransform::PositionAccess::WithY(float y) const {
	return glm::vec3(this->x, y, this->z);
}
glm::vec3 SceneTransform::PositionAccess::WithZ(float z) const {
	return glm::vec3(this->x, this->y, z);
}

SceneTransform::RotationAccess::RotationAccess(TransformAccess& source) :
source(source) {
	glm::vec3 scale = this->source.Scale();

	glm::mat3 rotationMatrix = (glm::mat3) this->source.transformation;

	rotationMatrix[0] /= scale.x;
	rotationMatrix[1] /= scale.y;
	rotationMatrix[2] /= scale.z;

	this->value = glm::normalize(glm::quat(rotationMatrix));
}

SceneTransform::RotationAccess::~RotationAccess() {
	glm::vec3 scale = this->source.Scale();

	glm::mat3 rotationMatrix = glm::mat3_cast(glm::normalize(this->value));

	rotationMatrix[0] *= scale.x;
	rotationMatrix[1] *= scale.y;
	rotationMatrix[2] *= scale.z;

	if (rotationMatrix != (glm::mat3) this->source.transformation) {
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
	}
}

glm::quat SceneTransform::RotationAccess::Value() const {
	return this->value;
}

SceneTransform::RotationAccess::operator glm::quat() const {
	return Value();
}

SceneTransform::RotationAccess& SceneTransform::RotationAccess::operator=(const glm::quat& rotation) {
	this->value = rotation;

	return *this;
}

SceneTransform::RotationAccess& SceneTransform::RotationAccess::operator*=(const glm::quat& rotation) {
	this->value *= rotation;

	return *this;
}

SceneTransform::RotationAccess& SceneTransform::RotationAccess::operator=(const glm::vec3& rotationEuler) {
	this->value = glm::quat(rotationEuler);

	return *this;
}

SceneTransform::RotationAccess& SceneTransform::RotationAccess::operator*=(const glm::vec3& rotationEuler) {
	this->value *= glm::quat(rotationEuler);;

	return *this;
}

SceneTransform::ScaleAccess::ScaleAccess(TransformAccess& source) :
source(source),
value(
	glm::length(glm::column(this->source.transformation, 0)),
	glm::length(glm::column(this->source.transformation, 1)),
	glm::length(glm::column(this->source.transformation, 2))
) { }

SceneTransform::ScaleAccess::~ScaleAccess() {
	glm::vec3 oldScale = glm::vec3(
		glm::length(glm::column(this->source.transformation, 0)),
		glm::length(glm::column(this->source.transformation, 1)),
		glm::length(glm::column(this->source.transformation, 2))
	);

	if (this->value != oldScale) {
		this->source.transformation[0][0] /= oldScale.x;
		this->source.transformation[0][0] *= this->value.x;
		this->source.transformation[1][1] /= oldScale.y;
		this->source.transformation[1][1] *= this->value.y;
		this->source.transformation[2][2] /= oldScale.z;
		this->source.transformation[2][2] *= this->value.z;

		this->source.MarkDirty();
	}
}

glm::vec3 SceneTransform::ScaleAccess::Value() const {
	return this->value;
}

SceneTransform::ScaleAccess::operator glm::vec3() const {
	return Value();
}

SceneTransform::ScaleAccess& SceneTransform::ScaleAccess::operator=(const glm::vec3& scale) {
	this->value = scale;

	return *this;
}

SceneTransform::ScaleAccess& SceneTransform::ScaleAccess::operator*=(const glm::vec3& scale) {
	this->value *= scale;

	return *this;
}

SceneTransform::ScaleAccess& SceneTransform::ScaleAccess::operator/=(const glm::vec3& scale) {
	this->value *= 1.0f / scale;

	return *this;
}

SceneTransform::ScaleAccess& SceneTransform::ScaleAccess::operator*=(float scale) {
	this->value *= scale;

	return *this;
}

SceneTransform::ScaleAccess& SceneTransform::ScaleAccess::operator/=(float scale) {
	this->value /= scale;

	return *this;
}

glm::vec3 operator+(const SceneTransform::PositionAccess& lh, const glm::vec3& rh) {
	return lh.value + rh;
}
glm::vec3 operator+(const glm::vec3& lh, const SceneTransform::PositionAccess& rh) {
	return lh + rh.value;
}
glm::vec3 operator-(const SceneTransform::PositionAccess& lh, const glm::vec3& rh) {
	return lh.value - rh;
}
glm::vec3 operator-(const glm::vec3& lh, const SceneTransform::PositionAccess& rh) {
	return lh - rh.value;
}

glm::quat operator*(const SceneTransform::RotationAccess& lh, const glm::quat& rh) {
	return lh.value * rh;
}
glm::quat operator*(const glm::quat& lh, const SceneTransform::RotationAccess& rh) {
	return lh * rh.value;
}

glm::vec3 operator*(const SceneTransform::ScaleAccess& lh, const glm::vec3& rh) {
	return lh.value * rh;
}
glm::vec3 operator*(const glm::vec3& lh, const SceneTransform::ScaleAccess& rh) {
	return lh * rh.value;
}
glm::vec3 operator/(const SceneTransform::ScaleAccess& lh, const glm::vec3& rh) {
	return lh.value / rh;
}
glm::vec3 operator/(const glm::vec3& lh, const SceneTransform::ScaleAccess& rh) {
	return lh / rh.value;
}
glm::vec3 operator*(const SceneTransform::ScaleAccess& lh, float rh) {
	return lh.value * rh;
}
glm::vec3 operator*(float lh, const SceneTransform::ScaleAccess& rh) {
	return lh * rh.value;
}
glm::vec3 operator/(const SceneTransform::ScaleAccess& lh, float rh) {
	return lh.value / rh;
}
glm::vec3 operator/(float lh, const SceneTransform::ScaleAccess& rh) {
	return lh / rh.value;
}