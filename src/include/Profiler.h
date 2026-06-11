#pragma once

#include <chrono>
#include <stack>
#include <filesystem>
#include <map>

class Profiler {
private:
	static constexpr int FrameMemory = 10;

	struct ProfilerData {
		std::filesystem::path name = "";
		std::chrono::nanoseconds times[FrameMemory] = {};
		int count;

		ProfilerData() = default;
		ProfilerData(const std::filesystem::path& name);
	};

	static std::map<std::filesystem::path, ProfilerData> profilerNodes;
	static std::stack<ProfilerData> profilerStack;
public:
	struct ProfilerResult {
		std::filesystem::path name = "";
		double time;
		int count;
	};

	static void Step();

	static void Push(const std::string& name);

	static void Pop();

	static std::vector<ProfilerResult> GrabResults();
};
