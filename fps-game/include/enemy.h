#pragma once
#include "raylib.h"
#include "raymath.h"

struct Level;
struct Player;
struct ParticleSystem;

enum class EnemyType {
    GRUNT,      // Basic enemy, slow, low health
    SOLDIER,    // Medium, has rifle
    DEMON,      // Fast melee
    HEAVY       // Slow, high health, heavy damage
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

    EnemyType type = EnemyType::GRUNT;
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

    void Init(EnemyType t, Vector3 pos);
    void Update(float dt, Player* player, Level* level, ParticleSystem* particles);
    void TakeDamage(int amount, Vector3 hitDir, ParticleSystem* particles);
    void Draw();
    bool IsAlive() const { return health > 0; }
    bool CanSeePlayer(Player* player, Level* level) const;

private:
    void UpdateIdle(float dt, Player* player, Level* level);
    void UpdatePatrol(float dt, Player* player, Level* level);
    void UpdateChase(float dt, Player* player, Level* level);
    void UpdateAttack(float dt, Player* player, ParticleSystem* particles);
    void PickPatrolTarget(Level* level);
    void MoveToward(Vector3 target, float dt, Level* level);
};
