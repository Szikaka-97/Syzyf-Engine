#pragma once

#include <memory>
#include <vector>

namespace Editor {
class ICommand {
  public:
    virtual ~ICommand() = default;
    virtual void Execute() = 0;
    virtual void Undo() = 0;

    virtual std::string GetName() const = 0;
};

class CommandHistory {
  private:
    std::vector<std::unique_ptr<ICommand>> history;
    int currentIndex = -1;

  public:
    void ExecuteCommand(std::unique_ptr<ICommand> command);

    void Undo();
    void Redo();

    bool CanUndo() const;
    bool CanRedo() const;

    const std::vector<std::unique_ptr<ICommand>>& GetHistorY() const;
    int GetCurrentIndex() const;
};
} // namespace Editor
