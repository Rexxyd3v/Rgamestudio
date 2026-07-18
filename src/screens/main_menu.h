#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <raylib.h>
#include "iscreen.h"
#include "menu_background.h"
#include "../constants.h"

class MainMenuScreen : public IScreen {
public:
    MainMenuScreen();
    ~MainMenuScreen();

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;

    GameMode GetSelectedMode() const { return selectedMode; }

private:
    Font menuFont;
    GameMode selectedMode;

    Rectangle cardOffline;
    Rectangle cardOnline;

    float hoverOffline;
    float hoverOnline;

    Music mainMenuMusic;
    Sound choiceSound;

    MenuBackground* background;

    float fadeInTimer;
    float blurStrength;
    float uiRise;
};

#endif // MAIN_MENU_H
