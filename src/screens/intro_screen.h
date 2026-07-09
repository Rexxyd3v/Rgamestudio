#ifndef FIRST_SCREEN_H
#define FIRST_SCREEN_H

#include <raylib.h>
#include "iscreen.h"

class FirstScreen : public IScreen {
private:
    Font gamefont;
    float positionRx;
    float positionRy;

    float slideSpeed;
    float slideLimit;
    float fadeAlpha;
    float fadeSpeed;

    bool isFading;
    bool isWaiting;
    bool isSliding;
    bool showGamestudio;
    bool isFadingOut;
    bool waitBeforeFadeOut;

    float waitTime;
    float fadeOutWaitTime;
    float elapsedTime;
    float fadeOutElapsedTime;

    // Second screen animation
    Font secondFont;
    enum SecondScreenState {
        SECOND_SCREEN_EXPANDING,
        SECOND_SCREEN_TEXT_WAIT,
        SECOND_SCREEN_FADING_OUT,
        SECOND_SCREEN_DONE
    } secondState;

    float secondTopAndLeft;
    float secondBottomAndRight;
    bool secondTextVisible;
    float secondWaitTimer;
    float secondFadeAlpha; // 0-255 for black fade

public:
    FirstScreen();
    ~FirstScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;
};

#endif // FIRST_SCREEN_H
