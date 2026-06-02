#include <PersistentData.h>

std::vector<std::function<void (void)>> PersistentData::clearFunctions;

void PersistentData::ClearAll() {
	for (auto cleaner : clearFunctions) {
		cleaner();
	}
}