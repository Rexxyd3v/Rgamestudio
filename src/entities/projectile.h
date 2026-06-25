#ifndef PROJECTILE_H
#define PROJECTILE_H

#include <raylib.h>

class Projectile {
public:
    Projectile(Vector2 startPosition, Vector2 direction, float speed);
    ~Projectile();

    void    Update(float deltaTime);
    void    Draw();
    bool    IsActive() const;
    void    Deactivate();   // Force-deactivate on hit
    Vector2 GetPosition() const;

private:
    Vector2 position;
    Vector2 direction;
    float   speed;
    bool    active;
    float   maxDistance;
    float   distanceTraveled;
};

#endif // PROJECTILE_H
