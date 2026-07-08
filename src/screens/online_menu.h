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
    int activeInput; // 0 = none, 1 = username, 2 = join address
    std::string joinAddress;
    std::string username;
    int currentSkin;
    bool isConnecting;         // True while async join is in progress
    std::string connectError;  // Set when a join attempt fails
};
#endif // ONLINE_MENU_H
