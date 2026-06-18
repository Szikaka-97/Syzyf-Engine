#include <Random.h>

#include <cstdlib>
#include <time.h>

#include <glm/gtc/constants.hpp>

Random* Random::globalInstance = new Random();

Random::Random():
state(time(0)) { }

Random::Random(int seed):
state(seed) { }

float Random::RandomValue() {
	return globalInstance->Value();
}
float Random::RandomValue(float maxValue) {
	return globalInstance->Value(maxValue);
}
float Random::RandomValue(float minValue, float maxValue) {
	return globalInstance->Value(minValue, maxValue);
}

float Random::Value() {
	srand(this->state);

	int val = rand();

	this->state = val;

	return (float) val / INT_MAX;
}
float Random::Value(float maxValue) {
	srand(this->state);

	int val = rand();

	this->state = val;

	return ((float) val / INT_MAX) * maxValue;
}
float Random::Value(float minValue, float maxValue) {
	if (minValue > maxValue) {
		std::swap(minValue, maxValue);
	}

	srand(this->state);

	int val = rand();

	this->state = val;

	return glm::mix(minValue, maxValue, (float) val / RAND_MAX);
}

int Random::RandomValueInt() {
	return globalInstance->ValueInt();
}
int Random::RandomValueInt(int maxValue) {
	return globalInstance->ValueInt(maxValue);
}
int Random::RandomValueInt(int minValue, int maxValue) {
	return globalInstance->ValueInt(minValue, maxValue);
}

int Random::ValueInt() {
	srand(this->state);

	int val = rand();

	this->state = val;

	return val;
}
int Random::ValueInt(int maxValue) {
	srand(this->state);

	int val = rand();

	this->state = val;

	return val % maxValue;
}
int Random::ValueInt(int minValue, int maxValue) {
	if (minValue > maxValue) {
		std::swap(minValue, maxValue);
	}

	if (minValue == maxValue) {
		return minValue;
	}

	srand(this->state);

	int val = rand();

	this->state = val;

	return minValue + (val % (maxValue - minValue));
}

bool Random::RandomChance(float probability) {
	return globalInstance->Chance(probability);
}

bool Random::Chance(float probability) {
	return Value() <= probability;
}

glm::vec3 Random::RandomOnUnitSphere() {
	return globalInstance->OnUnitSphere();
}
glm::vec3 Random::RandomInUnitBox() {
	return globalInstance->InUnitBox();
}

glm::vec3 Random::OnUnitSphere() {
	float y = Value(-1.0f, 1.0f);
	float longitude = Value(0, 2.0f * glm::pi<float>());

	float sqrt = glm::sqrt(1.0f - y*y);

	float x = glm::cos(longitude) * sqrt;
	float z = glm::sin(longitude) * sqrt;

	return glm::vec3(x, y, z);
}
glm::vec3 Random::InUnitBox() {
	return glm::vec3(Value(-1, 1), Value(-1, 1), Value(-1, 1));
}