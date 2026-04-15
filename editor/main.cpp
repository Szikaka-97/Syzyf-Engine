#include "include/Application.h"

int main(int, char**) {
    Editor::Application app = Editor::Application();

    if (!app.Setup()) {
        return -1;
    }

    app.MainLoop();
    app.Terminate();

    return 0;
}
