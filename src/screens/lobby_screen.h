#ifndef LOBBY_SCREEN_H
#define LOBBY_SCREEN_H

#include "iscreen.h"
#include "menu_background.h"
#include "../ui/character_preview.h"
#include "../network/network_manager.h"
#include <string>
#include <raylib.h>

class LobbyScreen : public IScreen {
public:
    LobbyScreen();
    ~LobbyScreen() override;

    bool Update(float deltaTime) override;
    void Draw(RenderTexture2D target) override;
    bool ShouldStartGame() const { return startGame; }

private:
    bool startGame;
    bool isHost;
    std::string playerName;
    std::string lanAddress;
    int playerSkin;

    Font menuFont;
    MenuBackground* background;
    CharacterPreview* preview;

    float fadeIn;
    float blurStrength;
    float hoverReady;
    float hoverStart;
    float hoverCopy;
    float hoverGlobalBack;

    Rectangle shell;
    Rectangle rosterPanel;
    Rectangle readyBtn;
    Rectangle startBtn;
    Rectangle copyBtn;

    void DrawRoster();
};

#endif // LOBBY_SCREEN_H
