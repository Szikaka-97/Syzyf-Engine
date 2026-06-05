#include <Profiler.h>

#include <vector>

#include <chrono>

#include <spdlog/spdlog.h>

std::map<std::string, Profiler::ProfilerData> Profiler::profilerNodes;

std::stack<Profiler::ProfilerData> Profiler::profilerStack;

Profiler::ProfilerData::ProfilerData(const std::string& name):
name(name),
times() { }

void Profiler::Step() {
	for (auto& pair : profilerNodes) {
		auto& node = pair.second;

		for (int i = 0; i < FrameMemory - 1; i++) {
			node.times[i] = node.times[i + 1];
		}

		node.times[FrameMemory - 1] = std::chrono::nanoseconds(0);
	}
}

void Profiler::Push(const std::string &name) {
	std::string fullName;

	if (profilerStack.empty()) {
		fullName = name;
	}
	else {
		fullName = std::format("{}/{}", profilerStack.top().name, name);
	}

	profilerStack.push(ProfilerData(fullName));

	ProfilerData& node = profilerStack.top();

	node.times[FrameMemory - 1] = std::chrono::high_resolution_clock::now().time_since_epoch();
}

void Profiler::Pop() {
	ProfilerData& node = profilerStack.top();

	node.times[FrameMemory - 1] = std::chrono::high_resolution_clock::now().time_since_epoch() - node.times[FrameMemory - 1];

	auto it = profilerNodes.find(node.name);

	if (it != profilerNodes.end()) {
		it->second.times[FrameMemory - 1] += node.times[FrameMemory - 1];
	}
	else {
		profilerNodes[node.name] = node;
	}

	profilerStack.pop();
}

std::vector<Profiler::ProfilerResult> Profiler::GrabResults() {
	std::vector<Profiler::ProfilerResult> result;
	result.reserve(profilerNodes.size());

	for (auto& pair : profilerNodes) {
		double eventTime = 0;

		for (int i = 0; i < FrameMemory; i++) {
			eventTime += (double) pair.second.times[i].count() / 1'000'000'000.0;
		}

		result.push_back({ pair.first, eventTime / FrameMemory });
	}

	return result;
}
