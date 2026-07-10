#ifndef CHARACTER_H
#define CHARACTER_H

#include <raylib.h>
#include "animation.h"
#include "projectile.h"
#include <map>
#include <string>
#include <vector>
#include <memory>

enum class CharState {
    IDLE,
    WALK,
    JUMP_START,
    JUMP_END,
    FALL,
    HIT,
    DEATH
};

class Character {
public:
    Character(Vector2 startPosition, const std::string& assetPath, float scale);
    virtual ~Character();

    virtual void Update(float deltaTime) = 0;
    virtual void Draw();

    Vector2 GetPosition() const   { return position; }
    void    SetPosition(Vector2 pos) { position = pos; }
    int     GetFaceDirection() const { return faceDirection; }
    void    SetFaceDirection(int dir) { faceDirection = dir; }
    void    SetAimTarget(Vector2 target) { aimTarget = target; }
    Vector2 GetAimTarget() const { return aimTarget; }
    void    ResetHealth(float amount) {
        health = amount;
        healthDisplay = amount;
        currentShootCooldown = 0.0f;
        jumpHeight = 0.0f;
        jumpVelocity = 0.0f;
        SetState(CharState::IDLE);
        if (animations.count(CharState::IDLE)) animations[CharState::IDLE]->Reset();
    }
    bool    IsDead() const { return health <= 0; }
    bool    IsMonster() const { return isMonster; }
    bool    IsAnimationFinished() const;
    void    TakeDamage(float amount);
    float   GetHealth() const;

    // Stats
    std::string GetName() const { return name; }
    void SetName(const std::string& n) { name = n; }
    int GetKills() const { return kills; }
    int GetDeaths() const { return deaths; }
    void AddKill() { kills++; }
    void AddDeath() { deaths++; }
    float GetShootCooldown() const { return currentShootCooldown; }
    void SetShootCooldown(float val) { currentShootCooldown = val; }

    // Skills
    float GetDashCooldown() const { return dashCooldown; }
    float GetDashTimer() const { return dashTimer; }
    float GetShieldCooldown() const { return shieldCooldown; }
    bool ActivateDash(Vector2 direction);
    bool ActivateShield();
    void UpdateSkills(float deltaTime);

    // Weapons and Reloading
    int GetAmmo() const;
    int GetMaxAmmo() const;
    bool IsReloading() const;
    void TriggerReload();

    // Platform / jump system
    float GetJumpHeight()    const { return jumpHeight; }
    void  SetJumpHeight(float h)   { jumpHeight = h; }
    float GetJumpVelocity()  const { return jumpVelocity; }
    void  SetJumpVelocity(float v) { jumpVelocity = v; }
    float GetBaseHeight()    const { return baseHeight; }
    void  SetBaseHeight(float h)   { baseHeight = h; }

    // floorHeight: 0 = flat ground, >0 = standing on an elevated platform
    float GetFloorHeight() const { return floorHeight; }
    void  SetFloorHeight(float h){ floorHeight = h; }

    void UpdatePhysics(float deltaTime); // uses internal floorHeight

    std::vector<std::shared_ptr<Projectile>>& GetProjectiles() { return projectiles; }
    void SwitchWeapon(int index);
    bool Shoot(Vector2 targetPos); // expose for remote player
    void ShootInDirection(Vector2 dir); // for remote players: uses normalized direction directly

    // Getters for networking
    int       GetWeaponIndex()  const { return currentWeaponIndex; }
    CharState GetCurrentState() const { return currentState; }
    bool      DidShoot() { bool ret = justShot; justShot = false; return ret; }
    Vector2   GetHandPosition() const;

protected:
    Vector2    position;
    Vector2    velocity;
    int        faceDirection; // 1 = right, -1 = left
    CharState  currentState;
    bool       justShot;
    bool       isMonster;
    std::string name;
    int        kills;
    int        deaths;
    float      muzzleFlashTimer;
    float      dashTimer;
    float      dashCooldown;
    Vector2    dashDirection;
    float      shieldTimer;
    float      shieldCooldown;
    int        ammo[3];
    float      reloadTimer;
    float      lastHitTimer;
    float      scale;
    float      health;
    float      healthDisplay; // for smooth health bar
    float      speed;
    float      shootCooldown;
    float      currentShootCooldown;

    float   jumpHeight;    // visual offset upward when jumping / on platform
    float   jumpVelocity;  // current vertical velocity
    float   baseHeight;    // (legacy, kept for compatibility)
    float   floorHeight;   // current floor level (0 = ground, rock.visualHeight = platform)
    Vector2 aimTarget;
    float   weaponRotation;

    std::map<CharState, Animation*>          animations;
    std::vector<std::shared_ptr<Projectile>> projectiles;
    std::vector<Texture2D>                   weaponTextures;
    int                                      currentWeaponIndex;

    void    LoadAnimations(const std::string& baseDir);
    void    SetState(CharState newState);
};

#endif // CHARACTER_H
