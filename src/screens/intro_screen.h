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

public:
    FirstScreen();
    ~FirstScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;
};

#endif // FIRST_SCREEN_H
