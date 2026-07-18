#ifndef MAP_CATALOG_H
#define MAP_CATALOG_H

#include <raylib.h>
#include <string>
#include <vector>
#include "../utils/texture_manager.h"
#include "ui_widgets.h"

enum class MapId {
    Beach  = 0,
    Space  = 1,
    Forest = 2,
    COUNT // keep last
};

// Returns the base path for the map (e.g., "assets/Maps/Beach/")
// Returns empty string for locked maps.
const char* MapPath(MapId id);

// Returns a vector of GalleryItem for the select gallery, with textures loaded.
// The caller should unload textures when done? We'll use TextureManager which manages refs.
// We'll return a static vector to avoid reloading each frame.
std::vector<Ui::GalleryItem> GetMapGalleryItems();

#endif // MAP_CATALOG_H