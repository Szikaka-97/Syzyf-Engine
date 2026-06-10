#pragma once

namespace Math {
	float MoveTowards(float current, float target, float maxDelta);
	float MoveTowardsAngle(float current, float target, float maxDelta);
	float MoveTowardsDegrees(float current, float target, float maxDelta);
};