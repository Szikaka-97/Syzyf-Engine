#pragma once

#include <string>

namespace Editor {
bool DrawRenameModal(const char* popupId, const std::string& targetName,
                     char* nameBuffer, size_t bufferSize);

bool DrawDeleteModal(const char* popupId, const std::string& targetName);
} // namespace Editor
