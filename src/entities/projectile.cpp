
#include "projectile.h"
#include <math.h>

Projectile::Projectile(Vector2 startPosition, Vector2 direction, float speed)
    : position(startPosition), direction(direction), speed(speed),
      active(true), maxDistance(1200.0f), distanceTraveled(0.0f) {
}

Projectile::~Projectile() {}

void Projectile::Update(float deltaTime) {
    if (!active) return;

    float stepX = speed * direction.x * deltaTime;
    float stepY = speed * direction.y * deltaTime;

    position.x += stepX;
    position.y += stepY;

    distanceTraveled += sqrtf(stepX*stepX + stepY*stepY);

    if (distanceTraveled >= maxDistance) {
        active = false;
    }
}

void Projectile::Draw() {
    if (!active) return;
    DrawCircle((int)position.x, (int)position.y, 4.0f, YELLOW);
    DrawCircleLines((int)position.x, (int)position.y, 4.5f, ORANGE);
}

void Projectile::Deactivate() {
    active = false;
}

bool Projectile::IsActive() const {
    return active;
}

Vector2 Projectile::GetPosition() const {
    return position;
}
