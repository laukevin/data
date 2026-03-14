#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

struct Weapon;

struct Player {
    Camera3D camera = {};
    Vector3 position = {0};
    Vector3 velocity = {0};
    float yaw = 0.0f;
    float pitch = 0.0f;

    // Stats
    int health = 100;
    int maxHealth = 100;
    int armor = 0;
    int maxArmor = 100;
    int ammo[4] = {50, 0, 0, 0};  // pistol, shotgun, rifle, rocket

    // Movement
    float moveSpeed = 8.0f;
    float sprintMultiplier = 1.6f;
    float jumpForce = 6.0f;
    float gravity = -20.0f;
    float playerHeight = 1.7f;
    float playerRadius = 0.3f;
    bool isGrounded = false;
    bool isSprinting = false;
    bool isCrouching = false;
    float crouchHeight = 1.0f;
    float currentHeight = 1.7f;

    // Footstep bob
    float bobTimer = 0.0f;
    float bobAmount = 0.03f;

    // Damage flash
    float damageFlash = 0.0f;

    // Current weapon index
    int currentWeapon = 0;

    void Init(Vector3 startPos);
    void Update(float dt, struct Level* level);
    void TakeDamage(int amount);
    bool IsAlive() const { return health > 0; }
    Vector3 GetForward() const;
    Vector3 GetRight() const;
    void AddHealth(int amount);
    void AddArmor(int amount);
};
