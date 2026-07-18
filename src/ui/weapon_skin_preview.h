#ifndef WEAPON_SKIN_PREVIEW_H
#define WEAPON_SKIN_PREVIEW_H

#include "ui_widgets.h"
#include "weapon_skin_catalog.h"

class WeaponSkinPreview {
public:
    WeaponSkinPreview(int skinId = 0);
    ~WeaponSkinPreview();

    void SetSkin(int skinId);
    void Update(float deltaTime);
    void DrawPreview(Rectangle area, int skinId, Font font);
    int DrawSelector(Rectangle area, Font font, int& selectedSkin, Vector2 mouse);


    int GetSelectedWeaponSlot() const { return selectedWeaponSlot; }
    void SetSelectedWeaponSlot(int slot) { selectedWeaponSlot = slot; }
    int GetWeaponSkin(int slot) const { return selectedSkins[slot]; }

private:
    int skinId;
    std::vector<Texture2D> weaponTextures;
    float rotation; // for gentle sway
    int selectedWeaponSlot; // 0 = SMG, 1 = Shotgun, 2 = Pistol
    int selectedSkins[3];   // Selected skin index per weapon
    float hoverPrev;
    float hoverNext;
    Rectangle lastArea;
};

#endif // WEAPON_SKIN_PREVIEW_H