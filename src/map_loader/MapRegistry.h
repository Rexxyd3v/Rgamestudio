#ifndef MAP_REGISTRY_H
#define MAP_REGISTRY_H

#include <string>
#include <unordered_map>
#include <vector>
#include "MapData.h"

// MapRegistry is a singleton that discovers and loads maps from the
// assets/Maps/ directory at runtime. Each subdirectory containing a .tmx file
// is registered as a playable map.
class MapRegistry {
public:
    static MapRegistry& GetInstance();

    // Scans assets/Maps/ for subdirectories, loads each .tmx file found,
    // and stores them in the internal map collection. Call once at startup.
    void LoadAllMaps(const char* mapsRoot = "assets/Maps/");

    // Retrieve a loaded map by name (case-sensitive folder name).
    // Returns nullptr if not found.
    MapData* GetMap(const std::string& name);

    // Get the map at a numeric index (for compatibility with old UI code).
    // Returns nullptr if index out of range.
    MapData* GetMapByIndex(int index);

    // Returns the number of loaded maps.
    int GetMapCount() const;

    // Returns a list of all map names (folder names).
    std::vector<std::string> GetMapNames() const;

    // Check if a map name exists.
    bool HasMap(const std::string& name) const;

    // Clear all loaded maps and free memory.
    void UnloadAll();

private:
    MapRegistry() = default;
    ~MapRegistry();
    MapRegistry(const MapRegistry&) = delete;
    MapRegistry& operator=(const MapRegistry&) = delete;

    std::unordered_map<std::string, MapData> maps;
    std::vector<std::string> mapOrder; // preserves insertion order
};

#endif // MAP_REGISTRY_H
