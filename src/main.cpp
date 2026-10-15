#include "game/game.h"

#include <cstdio>
#include <exception>

int main() {
    try {
        Game game;
        game.run();
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what());
        return 1;
    }

    return 0;
}
