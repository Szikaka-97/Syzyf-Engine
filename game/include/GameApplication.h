#pragma once

#include "Settings.h"
#include <Application.h>

class GameApplication : public ::Application {
  private:
    bool displayDebug = false;

  public:
    GameApplication() : ::Application("Bimberman", 1280, 720) {}

    void ApplySettings() override;
    Scene* CreateStartingScreenScene() override;

  protected:
    void OnInit(int argc, char* argv[]) override;
    void OnUpdate() override;
    void OnRender() override;
    void OnImGuiRender() override;
};
