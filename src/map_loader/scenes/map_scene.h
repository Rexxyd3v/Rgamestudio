#ifndef MAP_SCENE_H
#define MAP_SCENE_H

#include <raylib.h>
#include <vector>
#include <string>
#include <functional>
#include <unordered_map>

// ---------------------------------------------------------------------------
// MapScene — per-map world data that GameplayScreen and MenuBackground use.
//
// Each map registers a scene through MapSceneRegistry. The scene holds
// map-specific spawn points and configuration so that main_level.cpp and
// menu_background.cpp contain NO hard-coded map data.
// ---------------------------------------------------------------------------

struct MapSpawnPoint {
    Vector2 position;
};

struct MapScene {
    std::vector<MapSpawnPoint> spawnPoints;

    // Optional: texture filename relative to the map folder for fallback.
    std::string bgTexture = "background.png";

    bool useTmxBackground = true; // if true, DrawTMX is used
};

// ---------------------------------------------------------------------------
// MapSceneRegistry — maps a folder name (e.g. "Forest") to a build function.
// Call Register() from each scene's .cpp; GameplayScreen calls Get() at load.
// ---------------------------------------------------------------------------
class MapSceneRegistry {
public:
    using BuildFn = std::function<MapScene()>;

    static MapSceneRegistry& GetInstance() {
        static MapSceneRegistry inst;
        return inst;
    }

    void Register(const std::string& mapName, BuildFn fn) {
        builders[mapName] = fn;
    }

    // Returns a freshly built scene, or an empty one if no builder is registered.
    MapScene Build(const std::string& mapName) const {
        auto it = builders.find(mapName);
        if (it != builders.end()) return it->second();
        return MapScene{};
    }

    bool Has(const std::string& mapName) const {
        return builders.find(mapName) != builders.end();
    }

private:
    std::unordered_map<std::string, BuildFn> builders;
};

// ---------------------------------------------------------------------------
// Helper macro / RAII registrar so each scene .cpp can self-register.
// ---------------------------------------------------------------------------
struct MapSceneAutoRegister {
    MapSceneAutoRegister(const std::string& name, MapSceneRegistry::BuildFn fn) {
        MapSceneRegistry::GetInstance().Register(name, fn);
    }
};

#define REGISTER_MAP_SCENE(mapName, buildFn) \
    static MapSceneAutoRegister _sceneReg_##mapName(#mapName, buildFn)

#endif // MAP_SCENE_H
