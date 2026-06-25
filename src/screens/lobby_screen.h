#ifndef LOBBY_SCREEN_H
#define LOBBY_SCREEN_H

#include "iscreen.h"
#include "../network/network_manager.h"
#include <string>
#include <vector>

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
    int playerSkin;

    // Helper methods
    void UpdatePlayerList();
    void DrawPlayerList(float startX, float startY, float width, float height);
};

#endif // LOBBY_SCREEN_H