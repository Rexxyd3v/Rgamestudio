#include "weapon_skin_catalog.h"
#include <raylib.h>

#include <filesystem>

std::string GetWeaponSlotSkinPath(int slot, int skinId) {
    std::string defaultPath = "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Weapons/";
    if (skinId == 0) {
        return defaultPath;
    }
    
    std::string weaponFolderName = "SMG";
    if (slot == 1) weaponFolderName = "Shotgun";
    else if (slot == 2) weaponFolderName = "Pistol";
    
    std::string customPath = "assets/WeaponSkins/Skin" + std::to_string(skinId) + "/" + weaponFolderName + "/";
    if (std::filesystem::exists(customPath)) {
        return customPath;
    }
    
    return defaultPath;
}

Color GetWeaponSlotSkinTint(int slot, int skinId) {
    if (skinId == 0) return WHITE;
    
    std::string weaponFolderName = "SMG";
    if (slot == 1) weaponFolderName = "Shotgun";
    else if (slot == 2) weaponFolderName = "Pistol";
    
    std::string customPath = "assets/WeaponSkins/Skin" + std::to_string(skinId) + "/" + weaponFolderName + "/";
    if (std::filesystem::exists(customPath)) {
        return WHITE; // No tint if custom texture exists
    }
    
    // Fallback tints
    switch (skinId) {
        case 1: return { 110, 220, 255, 255 }; // Neon Camo / Classic Wood / Silver Alloy
        case 2: return { 255, 120, 70, 255 };  // Magma / Golden Oak / Neon Cyber
        case 3: return { 200, 200, 200, 255 }; // Carbon Fiber
        default: return WHITE;
    }
}

const char* WeaponSkinPath(WeaponSkinId id) {
    int skinId = static_cast<int>(id);
    
    // We define paths dynamically.
    // Use static std::string array so we can return a const char* safely.
    static std::string paths[8];
    if (skinId >= 0 && skinId < 8) {
        std::string folder = "assets/WeaponSkins/Skin" + std::to_string(skinId) + "/";
        if (std::filesystem::exists(folder)) {
            paths[skinId] = folder;
            return paths[skinId].c_str();
        }
    }
    
    return "assets/Free 2D Animated Vector Game Character Sprites/Free 2D Animated Vector Game Character Sprites/Weapons/";
}

Color GetWeaponSkinTint(int skinId) {
    std::string folder = "assets/WeaponSkins/Skin" + std::to_string(skinId) + "/";
    if (std::filesystem::exists(folder)) {
        return WHITE;
    }
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

int GetWeaponRenderFileNumber(int slot) {
    switch (slot) {
        case 1: return 3; // Shotgun slot -> weaponR3.png
        case 2: return 2; // Pistol slot  -> weaponR2.png
        default: return slot + 1; // SMG (slot 0) -> weaponR1.png
    }
}