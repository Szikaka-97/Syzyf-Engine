#include "CommandHistory.h"
#include <spdlog/spdlog.h>

namespace Editor {
void CommandHistory::ExecuteCommand(std::unique_ptr<ICommand> command) {
    if (currentIndex < static_cast<int>(history.size() - 1)) {
        history.erase(history.begin() + currentIndex + 1, history.end());
    }

    command->Execute();
    history.push_back(std::move(command));
    currentIndex++;
}

void CommandHistory::Undo() {
    if (CanUndo()) {
        history[currentIndex]->Undo();
        currentIndex--;
    }
}

void CommandHistory::Redo() {
    if (CanRedo()) {
        currentIndex++;
        history[currentIndex]->Execute();
    }
}

bool CommandHistory::CanUndo() const { return this->currentIndex >= 0; }

bool CommandHistory::CanRedo() const {
    return this->currentIndex < static_cast<int>(history.size()) - 1;
}
} // namespace Editor
