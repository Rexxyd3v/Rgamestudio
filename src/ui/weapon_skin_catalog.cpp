#include "weapon_skin_catalog.h"
#include <raylib.h>

const char* WeaponSkinPath(WeaponSkinId id) {
    (void)id;
    return "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Weapons/";
}

Color GetWeaponSkinTint(int skinId) {
    switch (skinId) {
        case 1: return { 110, 220, 255, 255 }; // Neon Camo
        case 2: return { 255, 120, 70, 255 };  // Magma / Flaming
        case 3: return { 200, 200, 200, 255 }; // Carbon Fiber
        default: return WHITE;
    }
}

std::vector<Ui::GalleryItem> GetWeaponSkinGalleryItems() {
    static std::vector<Ui::GalleryItem> items;
    static bool initialized = false;
    if (!initialized) {
        for (int i = 0; i < static_cast<int>(WeaponSkinId::COUNT); ++i) {
            WeaponSkinId id = static_cast<WeaponSkinId>(i);
            const char* path = WeaponSkinPath(id);
            bool locked = (id == WeaponSkinId::Locked1 || id == WeaponSkinId::Locked2);
            // Thumbnail texture: we can load a placeholder or a weapon icon
            Texture2D tex = {0};
            if (!locked && path && path[0] != '\0') {
                // For simplicity, we can load weaponR1.png as thumbnail
                std::string thumbPath = std::string(path) + "weaponR1.png";
                tex = TextureManager::GetTexture(thumbPath);
            }
            const char* label = "";
            switch (id) {
                case WeaponSkinId::Default: label = "Default"; break;
                case WeaponSkinId::Locked1: label = "Locked"; break;
                case WeaponSkinId::Locked2: label = "Locked"; break;
                default: label = "Unknown"; break;
            }
            items.push_back({ tex, label, locked });
        }
        initialized = true;
    }
    return items;
}