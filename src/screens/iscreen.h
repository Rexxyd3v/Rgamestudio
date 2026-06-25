#ifndef ISCREEN_H
#define ISCREEN_H

#include <raylib.h>

class IScreen {
public:
    virtual ~IScreen() {}
    virtual bool Update(float deltaTime) = 0;
    virtual void Draw(RenderTexture2D target) = 0;
};

#endif // ISCREEN_H
