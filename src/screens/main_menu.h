#ifndef MAIN_MENU_H
#define MAIN_MENU_H

#include <raylib.h>
#include "iscreen.h"
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

    Rectangle btn1Player;
    Rectangle btn2Player;

    bool hover1P;
    bool hover2P;

    Music mainMenuMusic;
    Sound choiceSound;

    bool shouldTransition;
};

#endif // MAIN_MENU_H
