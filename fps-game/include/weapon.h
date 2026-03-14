#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

struct Player;
struct Enemy;
struct Level;
struct ParticleSystem;

enum class WeaponType {
    PISTOL,
    SHOTGUN,
    RIFLE,
    COUNT
};

struct Projectile {
    Vector3 position;
    Vector3 velocity;
    float lifetime;
    float damage;
    float radius;
    bool active = true;
    Color color;
};

struct WeaponDef {
    WeaponType type;
    const char* name;
    int damage;
    float fireRate;       // shots per second
    int ammoPerShot;
    int ammoIndex;        // index into player ammo array
    float spread;         // accuracy spread in degrees
    int pellets;          // for shotgun
    float range;
    float kickback;       // view punch on fire
    bool automatic;
    Color muzzleColor;
};

struct WeaponSystem {
    static const int MAX_WEAPONS = 3;
    WeaponDef weapons[MAX_WEAPONS];
    bool owned[MAX_WEAPONS] = {true, false, false};

    // State
    float fireCooldown = 0.0f;
    float muzzleFlash = 0.0f;
    float recoilOffset = 0.0f;
    float swayTimer = 0.0f;
    float switchTimer = 0.0f;
    int switchTo = -1;

    // Projectiles
    std::vector<Projectile> projectiles;

    void Init();
    void Update(float dt, Player* player, std::vector<Enemy>& enemies,
                Level* level, ParticleSystem* particles);
    void Fire(Player* player, std::vector<Enemy>& enemies,
              Level* level, ParticleSystem* particles);
    void DrawWeaponModel(Player* player);
    void DrawProjectiles();
    void SwitchWeapon(int index, Player* player);

private:
    bool RaycastHit(Vector3 origin, Vector3 dir, float maxDist,
                    std::vector<Enemy>& enemies, Level* level,
                    Vector3& hitPoint, int& hitEnemyIdx);
};

extern WeaponSystem g_weapons;
