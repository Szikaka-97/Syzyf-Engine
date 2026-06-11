#include <MathHelpers.h>

#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

float Math::MoveTowards(float current, float target, float maxDelta) {
	maxDelta = glm::abs(maxDelta);

	if (current < target) {
		current += maxDelta;

		if (current > target) {
			return target;
		}
		else {
			return current;
		}
	}
	else {
		current -= maxDelta;

		if (current < target) {
			return target;
		}
		else {
			return current;
		}
	}
}
float Math::MoveTowardsAngle(float current, float target, float maxDelta) {
	current = glm::mod(current, glm::tau<float>());
	target = glm::mod(target, glm::tau<float>());

	maxDelta = glm::abs(maxDelta);

	if (glm::abs(target - current) <= glm::pi<float>()) {
		return MoveTowards(current, target, maxDelta);
	}
	else {
		if (current < target) {
			current += glm::tau<float>();
		}
		else {
			target += glm::tau<float>();
		}

		current = MoveTowards(current, target, maxDelta);

		return glm::mod(current, glm::tau<float>());
	}
}
float Math::MoveTowardsDegrees(float current, float target, float maxDelta) {
	current = glm::mod(current, 360.0f);
	target = glm::mod(target, 360.0f);

	maxDelta = glm::abs(maxDelta);

	if (glm::abs(target - current) <= 180.0f) {
		return MoveTowards(current, target, maxDelta);
	}
	else {
		if (current < target) {
			current += 360.0f;
		}
		else {
			target += 360.0f;
		}

		current = MoveTowards(current, target, maxDelta);

		return glm::mod(current, 360.0f);
	}
}