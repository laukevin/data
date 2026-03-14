#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

struct Level;
struct Player;
struct ParticleSystem;

enum class EnemyType {
    ZOMBIE_WALKER,   // Slow shambler
    ZOMBIE_RUNNER,   // Fast zombie
    ZOMBIE_BRUTE,    // Big tanky zombie
    ZOMBIE_SPITTER   // Ranged acid zombie
};

enum class EnemyState {
    IDLE,
    PATROL,
    CHASE,
    ATTACK,
    HURT,
    DEAD
};

struct Enemy {
    Vector3 position = {0};
    Vector3 velocity = {0};
    float yaw = 0.0f;
    float radius = 0.4f;
    float height = 1.8f;

    EnemyType type = EnemyType::ZOMBIE_WALKER;
    EnemyState state = EnemyState::IDLE;

    int health = 50;
    int maxHealth = 50;
    int damage = 10;
    float moveSpeed = 3.0f;
    float attackRange = 2.0f;
    float sightRange = 20.0f;
    float attackCooldown = 0.0f;
    float attackRate = 1.0f;
    float stateTimer = 0.0f;
    float hurtTimer = 0.0f;
    float deathTimer = 0.0f;
    bool active = true;

    // Patrol
    Vector3 patrolTarget = {0};
    float patrolWaitTimer = 0.0f;

    // Visual
    Color bodyColor = RED;
    Color eyeColor = YELLOW;

    // Co-op: track which player to target
    int targetPlayer = 0;  // 0 = local, 1 = remote

    void Init(EnemyType t, Vector3 pos);
    void Update(float dt, Player* player, Level* level, ParticleSystem* particles,
                Vector3* remotePlayerPos = nullptr, bool remoteAlive = false);
    void TakeDamage(int amount, Vector3 hitDir, ParticleSystem* particles);
    void Draw();
    bool IsAlive() const { return health > 0; }
    bool CanSeePlayer(Player* player, Level* level) const;
    bool CanSeePoint(Vector3 target, Level* level) const;

    // Get the closest player position (for co-op)
    Vector3 GetClosestTarget(Player* player, Vector3* remotePos, bool remoteAlive) const;

private:
    void UpdateIdle(float dt, Player* player, Level* level, Vector3* remotePos, bool remoteAlive);
    void UpdatePatrol(float dt, Player* player, Level* level, Vector3* remotePos, bool remoteAlive);
    void UpdateChase(float dt, Player* player, Level* level, Vector3* remotePos, bool remoteAlive);
    void UpdateAttack(float dt, Player* player, ParticleSystem* particles, Vector3* remotePos, bool remoteAlive);
    void PickPatrolTarget(Level* level);
    void MoveToward(Vector3 target, float dt, Level* level);
};
