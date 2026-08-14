// Forest map scene data.
// Spawn points and scene configuration for the Forest map live here.

#include "map_scene.h"

static MapScene BuildForestScene() {
    MapScene s;

    s.useTmxBackground = true;
    s.bgTexture        = "background.png";

    // ------------------------------------------------------------------
    // Spawn points — used for player/bot spawning in offline and online modes
    // ------------------------------------------------------------------
    s.spawnPoints = {
        {{ 300.0f,  300.0f}},
        {{1800.0f,  300.0f}},
        {{3500.0f,  300.0f}},
        {{ 300.0f, 1000.0f}},
        {{1800.0f, 1000.0f}},
        {{3500.0f, 1000.0f}},
        {{ 300.0f, 1700.0f}},
        {{1800.0f, 1700.0f}},
        {{3500.0f, 1700.0f}},
    };

    return s;
}

// Self-register so GameplayScreen and MenuBackground can look it up by name
REGISTER_MAP_SCENE(Forest, BuildForestScene);
