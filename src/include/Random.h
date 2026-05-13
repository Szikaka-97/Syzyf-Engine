#pragma once

#include <vector>
#include <glm/glm.hpp>

class Random {
private:
	static Random* globalInstance;
	
	int state;
public:
	Random();
	Random(int seed);

	static float RandomValue();
	static float RandomValue(float maxValue);
	static float RandomValue(float minValue, float maxValue);

	float Value();
	float Value(float maxValue);
	float Value(float minValue, float maxValue);

	static int RandomValueInt();
	static int RandomValueInt(int maxValue);
	static int RandomValueInt(int minValue, int maxValue);

	int ValueInt();
	int ValueInt(int maxValue);
	int ValueInt(int minValue, int maxValue);

	static bool RandomChance(float probability);

	bool Chance(float probability);

	static glm::vec3 RandomOnUnitSphere();
	static glm::vec3 RandomInUnitBox();

	glm::vec3 OnUnitSphere();
	glm::vec3 InUnitBox();

	template<typename T>
	static T& RandomPick(std::vector<T>& list);

	template<typename T>
	T& Pick(std::vector<T>& list);

	template<typename T>
	static std::vector<T> RandomShuffle(const std::vector<T>& list);

	template<typename T>
	std::vector<T> Shuffle(const std::vector<T>& list);
};

template<typename T>
T& Random::RandomPick(std::vector<T>& list) {
	return globalInstance->Pick(list);
}

template<typename T>
T& Random::Pick(std::vector<T>& list) {
	return list[ValueInt(list.size())];
}

template<typename T>
std::vector<T> Random::RandomShuffle(const std::vector<T>& list) {
	return globalInstance->Shuffle(list);
}

template<typename T>
std::vector<T> Random::Shuffle(const std::vector<T>& list) {
	std::vector<T> result(list.size());

	int n = list.size();

	while (n > 1) {
		n--;
		int k = ValueInt(n + 1);
		T value = list[k];
		result[k] = list[n];
		result[n] = value;
	}

	return result;
}