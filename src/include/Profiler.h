#pragma once

#include <chrono>
#include <stack>
#include <functional>
#include <map>

class Profiler {
private:
	static constexpr int FrameMemory = 10;

	struct ProfilerData {
		std::string name = "";
		std::chrono::nanoseconds times[FrameMemory] = {};

		ProfilerData() = default;
		ProfilerData(const std::string& name);
	};

	static std::map<std::string, ProfilerData> profilerNodes;
	static std::stack<ProfilerData> profilerStack;
public:
	struct ProfilerResult {
		std::string name = "";
		double time;
	};

	static void Step();

	static void Push(const std::string& name);

	static void Pop();

	static std::vector<ProfilerResult> GrabResults();
};
