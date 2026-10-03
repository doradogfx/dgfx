#include "game/game_app.h"

#include <cstdio>
#include <exception>

int main() {
    try {
        GameApp app;
        app.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
