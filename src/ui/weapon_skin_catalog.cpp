#include "weapon_skin_catalog.h"
#include <raylib.h>

const char* WeaponSkinPath(WeaponSkinId id) {
    switch (id) {
        case WeaponSkinId::Default: return "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Weapons/";
        case WeaponSkinId::Locked1: return ""; // locked
        case WeaponSkinId::Locked2: return ""; // locked
        default: return "";
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