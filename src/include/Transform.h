#pragma once

#include <concepts>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

class SceneNode;

class SceneTransform {
public: // Forward declarations
	class TransformAccess;
	class PositionAccess;
	class RotationAccess;
	class ScaleAccess;

	friend class SceneNode;
public:
	class TransformAccess {
		friend class SceneTransform;
		friend class PositionAccess;
		friend class RotationAccess;
		friend class ScaleAccess;
	private:
		bool dirty;
		SceneTransform& source;
		glm::mat4 transformation;
		
		TransformAccess(SceneTransform& source);
	public:
		void MarkDirty();
		bool IsDirty() const;

		PositionAccess Position();
		RotationAccess Rotation();
		ScaleAccess Scale();

		glm::vec3 Forward() const;
		glm::vec3 Backward() const;
		glm::vec3 Up() const;
		glm::vec3 Down() const;
		glm::vec3 Right() const;
		glm::vec3 Left() const;

		glm::mat4 Value() const;
		operator glm::mat4() const;

		TransformAccess& operator=(const TransformAccess&) = delete;
		TransformAccess& operator=(const glm::mat4& transformation);
	};

	class PositionAccess {
	private:
		TransformAccess& source;
	public:
		union {
			struct {
				float x;
				float y;
				float z;
			};
			glm::vec3 value;
		};

		PositionAccess(TransformAccess& source);
		~PositionAccess();

		glm::vec3 Value() const;
		operator glm::vec3() const;
		operator glm::vec2() const;

		PositionAccess& operator=(const glm::vec3& position);
		PositionAccess& operator+=(const glm::vec3& position);
		PositionAccess& operator-=(const glm::vec3& position);
		PositionAccess& operator=(const glm::vec2& position);
		PositionAccess& operator+=(const glm::vec2& position);
		PositionAccess& operator-=(const glm::vec2& position);

		PositionAccess& SetX(float x);
		PositionAccess& SetY(float y);
		PositionAccess& SetZ(float z);

		glm::vec3 WithX(float x) const;
		glm::vec3 WithY(float y) const;
		glm::vec3 WithZ(float z) const;
	};

	class RotationAccess {
	private:
		TransformAccess& source;
	public:
		union {
			struct {
				float x;
				float y;
				float z;
				float w;
			};
			glm::quat value;
		};

		RotationAccess(TransformAccess& source);
		~RotationAccess();

		glm::quat Value() const;
		operator glm::quat() const;
		
		RotationAccess& operator=(const glm::quat& rotation);
		RotationAccess& operator*=(const glm::quat& rotation);
		RotationAccess& operator=(const glm::vec3& rotationEuler);
		RotationAccess& operator*=(const glm::vec3& rotationEuler);
	};

	class ScaleAccess {
	private:
		TransformAccess& source;
	public:
		union {
			struct {
				float x;
				float y;
				float z;
			};
			glm::vec3 value;
		};

		ScaleAccess(TransformAccess& source);
		~ScaleAccess();

		glm::vec3 Value() const;
		operator glm::vec3() const;

		ScaleAccess& operator=(const glm::vec3& scale);
		ScaleAccess& operator*=(const glm::vec3& scale);
		ScaleAccess& operator/=(const glm::vec3& scale);
	};

	SceneTransform();
	SceneTransform(const glm::mat4& transformation);
	SceneTransform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

	TransformAccess& GlobalTransform();
	TransformAccess& LocalTransform();
	bool IsDirty() const;
	void ClearDirty();
private:
	TransformAccess globalTransform;
	TransformAccess localTransform;
	SceneNode* parent;
};

glm::vec3 operator+(const SceneTransform::PositionAccess& lh, const glm::vec3& rh);
glm::vec3 operator+(const glm::vec3& lh, const SceneTransform::PositionAccess& rh);
glm::vec3 operator-(const SceneTransform::PositionAccess& lh, const glm::vec3& rh);
glm::vec3 operator-(const glm::vec3& lh, const SceneTransform::PositionAccess& rh);

glm::quat operator*(const SceneTransform::RotationAccess& lh, const glm::quat& rh);
glm::quat operator*(const glm::quat& lh, const SceneTransform::RotationAccess& rh);

glm::vec3 operator*(const SceneTransform::ScaleAccess& lh, const glm::vec3& rh);
glm::vec3 operator*(const glm::vec3& lh, const SceneTransform::ScaleAccess& rh);
glm::vec3 operator/(const SceneTransform::ScaleAccess& lh, const glm::vec3& rh);
glm::vec3 operator/(const glm::vec3& lh, const SceneTransform::ScaleAccess& rh);