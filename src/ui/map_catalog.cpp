#include "map_catalog.h"
#include "../map_loader/MapRegistry.h"
#include <raylib.h>
#include <string>
#include <vector>
#include <cctype>

std::vector<Ui::GalleryItem> GetMapGalleryItems() {
    static std::vector<Ui::GalleryItem> items;
    static bool initialized = false;
    if (!initialized) {
        MapRegistry& registry = MapRegistry::GetInstance();
        auto mapNames = registry.GetMapNames();
        // Static vector to keep labels alive (GalleryItem uses const char*)
        static std::vector<std::string> persistentLabels;
        persistentLabels.clear();
        persistentLabels.reserve(mapNames.size());
        for (const auto& name : mapNames) {
            MapData* mapData = registry.GetMap(name);
            if (!mapData) continue;
            
            // Thumbnail texture: try to load preview.png from map folder
            Texture2D tex = {0};
            std::string previewPath = mapData->folderPath + "preview.png";
            if (FileExists(previewPath.c_str())) {
                tex = TextureManager::GetTexture(previewPath);
            }
            
            // Use folder name as label - capitalize first letter
            std::string labelStr = name;
            if (!labelStr.empty()) labelStr[0] = (char)toupper(labelStr[0]);
            persistentLabels.push_back(labelStr);
            
            items.push_back({ tex, persistentLabels.back().c_str(), false });
        }
        initialized = true;
    }
    return items;
}
