#ifndef WEAPON_SKIN_CATALOG_H
#define WEAPON_SKIN_CATALOG_H

#include <string>
#include <vector>
#include "../utils/texture_manager.h"
#include "ui_widgets.h" // for GalleryItem

enum class WeaponSkinId {
    Default = 0,
    Locked1 = 1,
    Locked2 = 2,
    COUNT // keep last
};

// Returns the prefix path for weapon textures for this skin.
// For example, if returns "assets/WeaponSkins/Default/", then weapon textures are
// <prefix>weaponR1.png, weaponR2.png, weaponR3.png.
// Returns empty string for locked skins.
const char* WeaponSkinPath(WeaponSkinId id);

// Returns a tint color for the weapon-specific skin selection so the player can
// see distinct visual variants even when only the shared default texture set exists.
Color GetWeaponSkinTint(int skinId);

// Returns a vector of GalleryItem for the select gallery.
std::vector<Ui::GalleryItem> GetWeaponSkinGalleryItems();

#endif // WEAPON_SKIN_CATALOG_H