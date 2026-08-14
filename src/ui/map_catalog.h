#ifndef MAP_CATALOG_H
#define MAP_CATALOG_H

#include <raylib.h>
#include <string>
#include <vector>
#include "../utils/texture_manager.h"
#include "ui_widgets.h"

// Returns a vector of GalleryItem for the select gallery, with textures loaded.
// The caller should unload textures when done? We'll use TextureManager which manages refs.
// We'll return a static vector to avoid reloading each frame.
std::vector<Ui::GalleryItem> GetMapGalleryItems();

#endif // MAP_CATALOG_H
