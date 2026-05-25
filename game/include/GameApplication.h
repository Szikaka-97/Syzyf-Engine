#pragma once

#include <Application.h>

class GameApplication : public ::Application {
  private:
    bool displayDebug = false;

  public:
    GameApplication() : ::Application("Bimberman", 1280, 720) {}

  protected:
    void OnInit() override;
    void OnUpdate() override;
    void OnRender() override;
    void OnImGuiRender() override;
};
