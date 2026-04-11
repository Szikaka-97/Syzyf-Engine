#include "include/Editor.h"
#include <Scene.h>

int main(int, char**) {
    if (!Editor::Setup()) {
        return -1;
    }

    Editor::MainLoop();
    Editor::Terminate();

    return 0;
}
