#ifndef MAP_DATA_H
#define MAP_DATA_H

#include <string>
#include <vector>
#include "raylib.h"
#include "raytmx.h"
#include "scenes/map_scene.h"

// Holds all data for a loaded map, wrapping the RayTMX TmxMap
// and deriving useful information from it.
struct MapData {
    std::string name;        // e.g. "Forest"
    std::string folderPath;  // e.g. "assets/Maps/Forest/"
    std::string tmxPath;     // e.g. "assets/Maps/Forest/map.tmx"
    TmxMap* tmxMap;          // The loaded TmxMap (owned by this struct)
    
    // Derived pixel dimensions (tileCount * tileSize)
    int pixelWidth;
    int pixelHeight;

    // World objects for this map (rocks, trees, spawn points).
    // Built by MapSceneRegistry from the corresponding scene .cpp file.
    MapScene scene;
    
    MapData() : tmxMap(nullptr), pixelWidth(0), pixelHeight(0) {}
    
    ~MapData() {
        if (tmxMap) {
            UnloadTMX(tmxMap);
            tmxMap = nullptr;
        }
    }
    
    // No copy — move only (TmxMap* is owned)
    MapData(const MapData&) = delete;
    MapData& operator=(const MapData&) = delete;
    
    MapData(MapData&& other) noexcept
        : name(std::move(other.name))
        , folderPath(std::move(other.folderPath))
        , tmxPath(std::move(other.tmxPath))
        , tmxMap(other.tmxMap)
        , pixelWidth(other.pixelWidth)
        , pixelHeight(other.pixelHeight)
        , scene(std::move(other.scene))
    {
        other.tmxMap = nullptr;
    }
    
    MapData& operator=(MapData&& other) noexcept {
        if (this != &other) {
            if (tmxMap) UnloadTMX(tmxMap);
            name = std::move(other.name);
            folderPath = std::move(other.folderPath);
            tmxPath = std::move(other.tmxPath);
            tmxMap = other.tmxMap;
            pixelWidth = other.pixelWidth;
            pixelHeight = other.pixelHeight;
            scene = std::move(other.scene);
            other.tmxMap = nullptr;
        }
        return *this;
    }
};

#endif // MAP_DATA_H
