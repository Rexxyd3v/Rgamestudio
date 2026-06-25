#ifndef ONLINE_MENU_H
#define ONLINE_MENU_H

#include "iscreen.h"
#include <string>

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
    int activeInput; // 0 = none, 1 = username, 2 = ip, 3 = room code
    std::string ipAddress;
    std::string username;
    int currentSkin;
    std::string roomCode; // For displaying room code when hosting
    bool isHosting;     // Track if we're currently hosting
};
#endif // ONLINE_MENU_H
