#ifndef CHARACTER_PREVIEW_H
#define CHARACTER_PREVIEW_H

#include <raylib.h>
#include "../entities/animation.h"
#include <string>

// Loads idle animations + head portraits for Char 1..4 and draws a selectable gallery.
class CharacterPreview {
public:
    static constexpr int kCount = 4;

    CharacterPreview();
    ~CharacterPreview();

    void Update(float deltaTime);

    // Draw large selected character at center of `stageBounds`.
    void DrawStage(Rectangle stageBounds, int selectedSkin /*1-4*/);

    // Draw 4 selectable cards. Returns clicked skin (1-4) or 0 if none.
    int DrawSelector(Rectangle area, const Font& font, int selectedSkin, Vector2 mouse);

    // Draw a single head portrait (skin 1-4) inside `bounds`.
    void DrawHead(int skin, Rectangle bounds) const;

    static std::string BodyPath(int skin);
    static std::string HeadPath(int skin);

private:
    Animation* idle[kCount] = {};
    Texture2D heads[kCount] = {};
    float pulse = 0.0f;
};

#endif // CHARACTER_PREVIEW_H
