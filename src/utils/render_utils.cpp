#include "render_utils.h"
#include "../constants.h"
#include <math.h>

void DrawScaledToScreen(RenderTexture2D target) {
    DrawTexturePro(
        target.texture,
        (Rectangle){0.0f, 0.0f, (float)target.texture.width, (float)-target.texture.height},
        (Rectangle){0.0f, 0.0f, (float)GetScreenWidth(), (float)GetScreenHeight()},
        (Vector2){0.0f, 0.0f},
        0.0f,
        WHITE
    );
}
