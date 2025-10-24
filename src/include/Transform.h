#pragma once

#include <glm/glm.hpp>

class SceneNode;

class Transform {
public: // Forward declarations
	class TransformAccess;
	class PositionAccess;
	class RotationAccess;
	class ScaleAccess;

	friend class SceneNode;
public:
	class TransformAccess {
		friend class Transform;
		friend class PositionAccess;
		friend class RotationAccess;
		friend class ScaleAccess;
	private:
		bool dirty;
		Transform& source;
		glm::mat4 transformation;
		
		TransformAccess(Transform& source);
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
		PositionAccess(TransformAccess& source);

		glm::vec3 Value() const;
		operator glm::vec3() const;
		
		PositionAccess& operator=(const glm::vec3& position);
	};
	class RotationAccess {
	private:
		TransformAccess& source;
	public:
		RotationAccess(TransformAccess& source);

		glm::quat Value() const;

		operator glm::quat() const;
		RotationAccess& operator=(const glm::quat& rotation);
	};
	class ScaleAccess {
	private:
		TransformAccess& source;
	public:
		ScaleAccess(TransformAccess& source);

		glm::vec3 Value() const;

		operator glm::vec3() const;
		ScaleAccess& operator=(const glm::vec3& scale);
	};

	Transform();
	Transform(const glm::mat4 transformation);
	Transform(const glm::vec3& translation, const glm::quat& rotation, const glm::vec3& scale);

	TransformAccess& GlobalTransform();
	TransformAccess& LocalTransform();
	bool IsDirty() const;
	void ClearDirty();
private:
	TransformAccess globalTransform;
	TransformAccess localTransform;
	SceneNode* parent;
};
