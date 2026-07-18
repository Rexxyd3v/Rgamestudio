#include "map_catalog.h"
#include <raylib.h>

const char* MapPath(MapId id) {
    switch (id) {
        case MapId::Beach:  return "assets/Maps/Beach/";
        case MapId::Space:  return "assets/Maps/Space/";
        case MapId::Forest: return "assets/Maps/Forest/";
        default: return "";
    }
}

std::vector<Ui::GalleryItem> GetMapGalleryItems() {
    static std::vector<Ui::GalleryItem> items;
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < static_cast<int>(MapId::COUNT); ++i) {
            MapId id = static_cast<MapId>(i);
            const char* path = MapPath(id);
            bool locked = false; // all maps now unlocked
            // Thumbnail texture: try to load preview.png from map folder
            Texture2D tex = {0};
            if (path && path[0] != '\0') {
                std::string previewPath = std::string(path) + "preview.png";
                if (FileExists(previewPath.c_str())) {
                    tex = TextureManager::GetTexture(previewPath);
                } else {
                    std::string fallbackPath = std::string(path) + "background.png";
                    tex = TextureManager::GetTexture(fallbackPath);
                }
            }
            const char* label = "";
            switch (id) {
                case MapId::Beach:  label = "BEACH";  break;
                case MapId::Space:  label = "SPACE";  break;
                case MapId::Forest: label = "FOREST"; break;
                default: label = "???"; break;
            }
            items.push_back({ tex, label, locked });
        }
        initialized = true;
    }
    return items;
}