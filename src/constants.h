#ifndef CONSTANTS_H
#define CONSTANTS_H

#define VIRTUAL_WIDTH 1300
#define VIRTUAL_HEIGHT 680
#define DEFAULT_GAME_PORT 7777

// World is 3x the virtual screen (must be visible to all screens that simulate world space)
#define WORLD_WIDTH  3900
#define WORLD_HEIGHT 2040

enum class GameMode {
    NONE,
    OFFLINE,
    ONLINE
};

#endif // CONSTANTS_H
