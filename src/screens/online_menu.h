#ifndef ONLINE_MENU_H
#define ONLINE_MENU_H

#include "iscreen.h"
#include "menu_background.h"
#include "../ui/character_preview.h"
#include <string>
#include <raylib.h>

class OnlineMenuScreen : public IScreen {
public:
    OnlineMenuScreen();
    ~OnlineMenuScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;

    bool ShouldStartGame() const { return startGame; }
    bool ShouldGoToLobby() const { return goToLobby; }

private:
    bool startGame;
    bool goToLobby;
    int activeInput; // 0 = none, 1 = username, 2 = join address
    std::string joinAddress;
    std::string username;
    int currentSkin;
    bool isConnecting;
    std::string connectError;

    Font menuFont;
    MenuBackground* background;
    CharacterPreview* preview;

    float fadeIn;
    float blurStrength;
    float hoverHost;
    float hoverJoin;
    float hoverGlobalBack;
    float caretBlink;

    Rectangle nameField;
    Rectangle hostBtn;
    Rectangle joinBtn;
    Rectangle shell;
    Rectangle stageArea;
    Rectangle selectorArea;
};

#endif // ONLINE_MENU_H
