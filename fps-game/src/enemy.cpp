#include "enemy.h"
#include "player.h"
#include "level.h"
#include "particles.h"
#include <cmath>

void Enemy::Init(EnemyType t, Vector3 pos) {
    type = t;
    position = pos;
    position.y = 0.0f;
    velocity = {0};
    state = EnemyState::PATROL;
    active = true;
    yaw = (float)GetRandomValue(0, 360);

    switch (type) {
        case EnemyType::ZOMBIE_WALKER:
            health = maxHealth = 40;
            damage = 10;
            moveSpeed = 2.0f;
            attackRange = 2.2f;
            sightRange = 18.0f;
            attackRate = 1.0f;
            bodyColor = {80, 120, 60, 255};   // Sickly green
            eyeColor = {200, 255, 100, 255};  // Glowing yellow-green
            radius = 0.35f;
            height = 1.6f;
            break;
        case EnemyType::ZOMBIE_RUNNER:
            health = maxHealth = 30;
            damage = 15;
            moveSpeed = 6.5f;
            attackRange = 2.0f;
            sightRange = 25.0f;
            attackRate = 0.5f;
            bodyColor = {100, 70, 70, 255};    // Dark reddish
            eyeColor = {255, 50, 50, 255};     // Red glow
            radius = 0.3f;
            height = 1.5f;
            break;
        case EnemyType::ZOMBIE_BRUTE:
            health = maxHealth = 200;
            damage = 30;
            moveSpeed = 1.5f;
            attackRange = 3.0f;
            sightRange = 20.0f;
            attackRate = 1.8f;
            bodyColor = {60, 70, 50, 255};     // Dark olive
            eyeColor = {255, 100, 0, 255};     // Orange glow
            radius = 0.6f;
            height = 2.4f;
            break;
        case EnemyType::ZOMBIE_SPITTER:
            health = maxHealth = 50;
            damage = 12;
            moveSpeed = 3.0f;
            attackRange = 18.0f;
            sightRange = 22.0f;
            attackRate = 1.5f;
            bodyColor = {70, 100, 70, 255};    // Toxic green
            eyeColor = {0, 255, 100, 255};     // Bright green
            radius = 0.35f;
            height = 1.7f;
            break;
    }

    patrolTarget = position;
    attackCooldown = 0.0f;
    targetPlayer = 0;
}

Vector3 Enemy::GetClosestTarget(Player* player, Vector3* remotePos, bool remoteAlive) const {
    Vector3 localPos = player->position;
    float distLocal = Vector3Distance(position, localPos);

    if (remotePos && remoteAlive) {
        float distRemote = Vector3Distance(position, *remotePos);
        if (distRemote < distLocal) {
            return *remotePos;
        }
    }
    return localPos;
}

void Enemy::Update(float dt, Player* player, Level* level, ParticleSystem* particles,
                   Vector3* remotePlayerPos, bool remoteAlive) {
    if (!active) return;

    if (!IsAlive()) {
        deathTimer += dt;
        if (deathTimer > 3.0f) {
            active = false;
        }
        return;
    }

    if (hurtTimer > 0) {
        hurtTimer -= dt;
        return;
    }

    attackCooldown -= dt;
    stateTimer += dt;

    switch (state) {
        case EnemyState::IDLE: UpdateIdle(dt, player, level, remotePlayerPos, remoteAlive); break;
        case EnemyState::PATROL: UpdatePatrol(dt, player, level, remotePlayerPos, remoteAlive); break;
        case EnemyState::CHASE: UpdateChase(dt, player, level, remotePlayerPos, remoteAlive); break;
        case EnemyState::ATTACK: UpdateAttack(dt, player, particles, remotePlayerPos, remoteAlive); break;
        default: break;
    }
}

void Enemy::UpdateIdle(float dt, Player* player, Level* level,
                       Vector3* remotePos, bool remoteAlive) {
    // Check if can see any player
    bool seesLocal = CanSeePlayer(player, level);
    bool seesRemote = false;
    if (remotePos && remoteAlive) {
        seesRemote = CanSeePoint(*remotePos, level) &&
                     Vector3Distance(position, *remotePos) < sightRange;
    }

    if (seesLocal || seesRemote) {
        state = EnemyState::CHASE;
        stateTimer = 0;
        return;
    }
    patrolWaitTimer -= dt;
    if (patrolWaitTimer <= 0) {
        PickPatrolTarget(level);
        state = EnemyState::PATROL;
        stateTimer = 0;
    }
}

void Enemy::UpdatePatrol(float dt, Player* player, Level* level,
                         Vector3* remotePos, bool remoteAlive) {
    bool seesLocal = CanSeePlayer(player, level);
    bool seesRemote = false;
    if (remotePos && remoteAlive) {
        seesRemote = CanSeePoint(*remotePos, level) &&
                     Vector3Distance(position, *remotePos) < sightRange;
    }

    if (seesLocal || seesRemote) {
        state = EnemyState::CHASE;
        stateTimer = 0;
        return;
    }

    float dist = Vector3Distance(position, patrolTarget);
    if (dist < 1.0f || stateTimer > 8.0f) {
        state = EnemyState::IDLE;
        patrolWaitTimer = GetRandomValue(10, 30) / 10.0f;
        stateTimer = 0;
        return;
    }

    MoveToward(patrolTarget, dt, level);
}

void Enemy::UpdateChase(float dt, Player* player, Level* level,
                        Vector3* remotePos, bool remoteAlive) {
    if (!player->IsAlive() && (!remoteAlive)) {
        state = EnemyState::IDLE;
        return;
    }

    Vector3 target = GetClosestTarget(player, remotePos, remoteAlive);
    float dist = Vector3Distance(position, target);

    if (dist > sightRange * 1.5f) {
        state = EnemyState::PATROL;
        PickPatrolTarget(level);
        stateTimer = 0;
        return;
    }

    if (dist <= attackRange) {
        state = EnemyState::ATTACK;
        stateTimer = 0;
        return;
    }

    MoveToward(target, dt, level);
}

void Enemy::UpdateAttack(float dt, Player* player, ParticleSystem* particles,
                         Vector3* remotePos, bool remoteAlive) {
    Vector3 target = GetClosestTarget(player, remotePos, remoteAlive);
    float dist = Vector3Distance(position, target);

    // Face target
    Vector3 dir = Vector3Subtract(target, position);
    yaw = atan2f(dir.x, dir.z) * RAD2DEG;

    if (dist > attackRange * 1.2f) {
        state = EnemyState::CHASE;
        stateTimer = 0;
        return;
    }

    if (attackCooldown <= 0) {
        attackCooldown = attackRate;

        // Determine which player is closest and damage them
        float distLocal = Vector3Distance(position, player->position);
        float distRemote = (remotePos && remoteAlive) ?
                            Vector3Distance(position, *remotePos) : 999.0f;

        if (distLocal <= distRemote && distLocal <= attackRange * 1.2f) {
            player->TakeDamage(damage);
        }
        // Remote player damage is handled by host sending PLAYER_DAMAGED

        // Visual feedback
        if (type == EnemyType::ZOMBIE_WALKER || type == EnemyType::ZOMBIE_RUNNER ||
            type == EnemyType::ZOMBIE_BRUTE) {
            // Melee claw swipe
            Vector3 attackPos = Vector3Add(position,
                Vector3Scale(Vector3Normalize(dir), 1.0f));
            attackPos.y = 1.0f;
            particles->EmitSpark(attackPos, Vector3Normalize(dir), 5);
        } else {
            // Spitter: acid projectile effect
            Vector3 muzzle = position;
            muzzle.y = height * 0.7f;
            Vector3 fwd = Vector3Normalize(dir);
            muzzle = Vector3Add(muzzle, Vector3Scale(fwd, 0.5f));
            particles->EmitMuzzleFlash(muzzle, fwd);
        }
    }
}

void Enemy::TakeDamage(int amount, Vector3 hitDir, ParticleSystem* particles) {
    if (!IsAlive()) return;

    health -= amount;
    hurtTimer = 0.15f;

    Vector3 bloodPos = position;
    bloodPos.y = height * 0.5f;
    particles->EmitBlood(bloodPos, hitDir, 8);

    if (health <= 0) {
        health = 0;
        state = EnemyState::DEAD;
        deathTimer = 0.0f;
        particles->EmitBlood(bloodPos, {0, 1, 0}, 15);
    } else {
        if (state == EnemyState::IDLE || state == EnemyState::PATROL) {
            state = EnemyState::CHASE;
        }
    }
}

void Enemy::Draw() {
    if (!active) return;

    float alpha = 1.0f;
    float yOffset = 0.0f;

    if (!IsAlive()) {
        float t = deathTimer / 3.0f;
        alpha = 1.0f - t;
        yOffset = -t * (height * 0.5f);
    }

    Color body = bodyColor;
    if (hurtTimer > 0) body = WHITE;
    body.a = (unsigned char)(alpha * 255);

    float drawY = yOffset;

    // Body (capsule shape) - zombie proportions
    DrawCapsule(
        {position.x, drawY + radius, position.z},
        {position.x, drawY + height - radius * 0.5f, position.z},
        radius, 4, 4, body
    );

    // Zombie details
    if (IsAlive()) {
        float fwd_x = sinf(yaw * DEG2RAD);
        float fwd_z = cosf(yaw * DEG2RAD);

        // Glowing eyes
        float eyeY = drawY + height * 0.75f;
        float right_x = cosf(yaw * DEG2RAD) * 0.12f;
        float right_z = -sinf(yaw * DEG2RAD) * 0.12f;

        Color eye = eyeColor;
        eye.a = (unsigned char)(alpha * 255);

        DrawSphere({position.x + fwd_x * (radius + 0.05f) + right_x,
                    eyeY,
                    position.z + fwd_z * (radius + 0.05f) + right_z},
                   0.07f, eye);
        DrawSphere({position.x + fwd_x * (radius + 0.05f) - right_x,
                    eyeY,
                    position.z + fwd_z * (radius + 0.05f) - right_z},
                   0.07f, eye);

        // Brute: extra bulk
        if (type == EnemyType::ZOMBIE_BRUTE) {
            Color darkBody = body;
            darkBody.r = (unsigned char)(darkBody.r * 0.7f);
            darkBody.g = (unsigned char)(darkBody.g * 0.7f);
            DrawSphere({position.x, drawY + height * 0.5f, position.z},
                       radius * 1.3f, darkBody);
        }

        // Spitter: glowing mouth
        if (type == EnemyType::ZOMBIE_SPITTER) {
            DrawSphere({position.x + fwd_x * (radius + 0.1f),
                        drawY + height * 0.6f,
                        position.z + fwd_z * (radius + 0.1f)},
                       0.1f, GREEN);
        }

        // Arms (simple cylinders extending forward during attack)
        if (state == EnemyState::ATTACK || state == EnemyState::CHASE) {
            float armExtend = (state == EnemyState::ATTACK) ? 0.6f : 0.3f;
            float armY = drawY + height * 0.55f;

            Color armColor = body;
            armColor.r = (unsigned char)(armColor.r * 0.85f);
            armColor.g = (unsigned char)(armColor.g * 0.85f);
            armColor.b = (unsigned char)(armColor.b * 0.85f);

            // Left arm
            DrawCapsule(
                {position.x + right_x * 2, armY, position.z + right_z * 2},
                {position.x + right_x * 2 + fwd_x * armExtend, armY - 0.1f,
                 position.z + right_z * 2 + fwd_z * armExtend},
                0.08f, 3, 3, armColor
            );
            // Right arm
            DrawCapsule(
                {position.x - right_x * 2, armY, position.z - right_z * 2},
                {position.x - right_x * 2 + fwd_x * armExtend, armY - 0.1f,
                 position.z - right_z * 2 + fwd_z * armExtend},
                0.08f, 3, 3, armColor
            );
        }
    }

    // Health bar
    if (IsAlive() && health < maxHealth) {
        float barWidth = 0.8f;
        float healthFrac = (float)health / maxHealth;
        Vector3 barPos = {position.x, drawY + height + 0.3f, position.z};
        DrawCube(barPos, barWidth, 0.08f, 0.08f, DARKGRAY);
        float hWidth = barWidth * healthFrac;
        Color hColor = healthFrac > 0.5f ? GREEN : (healthFrac > 0.25f ? YELLOW : RED);
        DrawCube({barPos.x - (barWidth - hWidth) * 0.5f, barPos.y, barPos.z},
                 hWidth, 0.08f, 0.08f, hColor);
    }
}

bool Enemy::CanSeePlayer(Player* player, Level* level) const {
    float dist = Vector3Distance(position, player->position);
    if (dist > sightRange) return false;
    return CanSeePoint(player->position, level);
}

bool Enemy::CanSeePoint(Vector3 target, Level* level) const {
    Vector3 start = position;
    start.y = height * 0.7f;
    Vector3 end = target;
    end.y = 1.5f;

    Vector3 dir = Vector3Subtract(end, start);
    float len = Vector3Length(dir);
    dir = Vector3Normalize(dir);

    float step = 0.5f;
    for (float t = step; t < len; t += step) {
        Vector3 p = Vector3Add(start, Vector3Scale(dir, t));
        if (level->IsWall(p.x, p.z)) return false;
    }
    return true;
}

void Enemy::PickPatrolTarget(Level* level) {
    for (int i = 0; i < 20; i++) {
        float angle = GetRandomValue(0, 360) * DEG2RAD;
        float dist = GetRandomValue(30, 80) / 10.0f;
        Vector3 target = {
            position.x + cosf(angle) * dist,
            0,
            position.z + sinf(angle) * dist
        };
        if (!level->IsWall(target.x, target.z)) {
            patrolTarget = target;
            return;
        }
    }
    patrolTarget = position;
}

void Enemy::MoveToward(Vector3 target, float dt, Level* level) {
    Vector3 dir = Vector3Subtract(target, position);
    dir.y = 0;
    float dist = Vector3Length(dir);
    if (dist < 0.1f) return;

    dir = Vector3Normalize(dir);
    yaw = atan2f(dir.x, dir.z) * RAD2DEG;

    Vector3 newPos = position;
    newPos.x += dir.x * moveSpeed * dt;
    newPos.z += dir.z * moveSpeed * dt;

    newPos = level->ResolveCollision(position, newPos, radius);
    position = newPos;
}
