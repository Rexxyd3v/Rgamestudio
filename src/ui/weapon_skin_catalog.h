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

// Returns the prefix path for weapon textures for this skin and weapon slot.
std::string GetWeaponSlotSkinPath(int slot, int skinId);

// Returns the tint color for the weapon-specific skin selection and slot.
Color GetWeaponSlotSkinTint(int slot, int skinId);

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

// Returns the correct weaponR<N>.png file number for a given weapon slot
// (0 = SMG, 1 = Shotgun, 2 = Pistol). Exists because the Shotgun/Pistol
// default sprite files were swapped at the asset level (weaponR2.png
// contains pistol art, weaponR3.png contains shotgun art).
int GetWeaponRenderFileNumber(int slot);

#endif // WEAPON_SKIN_CATALOG_H