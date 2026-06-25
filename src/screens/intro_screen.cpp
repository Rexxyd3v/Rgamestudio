#include "intro_screen.h"
#include "../utils/render_utils.h"
#include "../constants.h"
#include <unistd.h>

FirstScreen::FirstScreen() {
    gamefont = LoadFontEx("assets/fonts/BruceForeverRegular-X3jd2.ttf", 260, nullptr, 0);
    positionRx = 520.0f;
    positionRy = 150.0f;

    slideSpeed = 7.0f;
    slideLimit = 340.0f;
    fadeAlpha = 0.0f;
    fadeSpeed = 6.0f;

    isFading = true;
    isWaiting = false;
    isSliding = false;
    showGamestudio = false;
    isFadingOut = false;
    waitBeforeFadeOut = false;

    waitTime = 1.0f;
    fadeOutWaitTime = 2.0f;
    elapsedTime = 0.0f;
    fadeOutElapsedTime = 0.0f;
}

FirstScreen::~FirstScreen() {
    UnloadFont(gamefont);
}

bool FirstScreen::Update(float deltaTime) {
    if (isFading) {
        fadeAlpha += fadeSpeed * (deltaTime * 60.0f);
        if (fadeAlpha >= 255.0f) {
            fadeAlpha = 255.0f;
            isFading = false;
            isWaiting = true;
        }
    } else if (isWaiting) {
        elapsedTime += deltaTime;
        if (elapsedTime >= waitTime) {
            isWaiting = false;
            isSliding = true;
        }
    } else if (isSliding) {
        if (positionRx > slideLimit) {
            positionRx -= slideSpeed * (deltaTime * 60.0f);
        } else {
            isSliding = false;
            showGamestudio = true;
            waitBeforeFadeOut = true;
        }
    } else if (waitBeforeFadeOut) {
        fadeOutElapsedTime += deltaTime;
        if (fadeOutElapsedTime >= fadeOutWaitTime) {
            waitBeforeFadeOut = false;
            isFadingOut = true;
        }
    } else if (isFadingOut) {
        fadeAlpha -= fadeSpeed * (deltaTime * 60.0f);
        if (fadeAlpha <= 0.0f) {
            fadeAlpha = 0.0f;
            isFadingOut = false;
            return false; // Transition to next screen
        }
    }
    return true; // Keep running this screen
}

void FirstScreen::Draw(RenderTexture2D target) {
    BeginTextureMode(target);
    ClearBackground(BLACK);
    
    Color drawColor = {255, 255, 255, (unsigned char)fadeAlpha};
    Vector2 rPos = {positionRx, positionRy};
    
    DrawTextEx(gamefont, "R", rPos, 260.0f, 2.0f, drawColor);
    
    if (showGamestudio) {
        DrawLine(560, 310, 970, 310, drawColor);
        Vector2 studioPos = {560.0f, 260.0f};
        DrawTextEx(gamefont, "Gamestudio.", studioPos, 50.0f, 2.0f, drawColor);
    }
    EndTextureMode();
}
