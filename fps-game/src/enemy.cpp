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
        case EnemyType::GRUNT:
            health = maxHealth = 40;
            damage = 8;
            moveSpeed = 2.5f;
            attackRange = 15.0f;
            sightRange = 18.0f;
            attackRate = 1.2f;
            bodyColor = {180, 80, 80, 255};
            eyeColor = YELLOW;
            radius = 0.35f;
            height = 1.6f;
            break;
        case EnemyType::SOLDIER:
            health = maxHealth = 70;
            damage = 12;
            moveSpeed = 3.5f;
            attackRange = 20.0f;
            sightRange = 25.0f;
            attackRate = 0.8f;
            bodyColor = {80, 100, 80, 255};
            eyeColor = RED;
            radius = 0.4f;
            height = 1.8f;
            break;
        case EnemyType::DEMON:
            health = maxHealth = 60;
            damage = 20;
            moveSpeed = 6.0f;
            attackRange = 2.5f;
            sightRange = 22.0f;
            attackRate = 0.6f;
            bodyColor = {150, 50, 50, 255};
            eyeColor = {255, 100, 0, 255};
            radius = 0.45f;
            height = 1.5f;
            break;
        case EnemyType::HEAVY:
            health = maxHealth = 150;
            damage = 25;
            moveSpeed = 1.8f;
            attackRange = 18.0f;
            sightRange = 22.0f;
            attackRate = 1.5f;
            bodyColor = {100, 80, 60, 255};
            eyeColor = {255, 50, 50, 255};
            radius = 0.55f;
            height = 2.2f;
            break;
    }

    patrolTarget = position;
    attackCooldown = 0.0f;
}

void Enemy::Update(float dt, Player* player, Level* level, ParticleSystem* particles) {
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
        case EnemyState::IDLE: UpdateIdle(dt, player, level); break;
        case EnemyState::PATROL: UpdatePatrol(dt, player, level); break;
        case EnemyState::CHASE: UpdateChase(dt, player, level); break;
        case EnemyState::ATTACK: UpdateAttack(dt, player, particles); break;
        default: break;
    }
}

void Enemy::UpdateIdle(float dt, Player* player, Level* level) {
    if (CanSeePlayer(player, level)) {
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

void Enemy::UpdatePatrol(float dt, Player* player, Level* level) {
    if (CanSeePlayer(player, level)) {
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

void Enemy::UpdateChase(float dt, Player* player, Level* level) {
    if (!player->IsAlive()) {
        state = EnemyState::IDLE;
        return;
    }

    float dist = Vector3Distance(position, player->position);

    if (dist > sightRange * 1.5f && !CanSeePlayer(player, level)) {
        state = EnemyState::PATROL;
        PickPatrolTarget(level);
        stateTimer = 0;
        return;
    }

    if (dist <= attackRange && CanSeePlayer(player, level)) {
        state = EnemyState::ATTACK;
        stateTimer = 0;
        return;
    }

    MoveToward(player->position, dt, level);
}

void Enemy::UpdateAttack(float dt, Player* player, ParticleSystem* particles) {
    if (!player->IsAlive()) {
        state = EnemyState::IDLE;
        return;
    }

    float dist = Vector3Distance(position, player->position);

    // Face player
    Vector3 dir = Vector3Subtract(player->position, position);
    yaw = atan2f(dir.x, dir.z) * RAD2DEG;

    if (dist > attackRange * 1.2f) {
        state = EnemyState::CHASE;
        stateTimer = 0;
        return;
    }

    if (attackCooldown <= 0) {
        // Attack!
        player->TakeDamage(damage);
        attackCooldown = attackRate;

        // Visual feedback
        if (type == EnemyType::DEMON) {
            // Melee slash effect
            Vector3 attackPos = Vector3Add(position, Vector3Scale(Vector3Normalize(dir), 1.0f));
            attackPos.y = 1.0f;
            particles->EmitSpark(attackPos, Vector3Normalize(dir), 5);
        } else {
            // Ranged - muzzle flash
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

    // Blood particles
    Vector3 bloodPos = position;
    bloodPos.y = height * 0.5f;
    particles->EmitBlood(bloodPos, hitDir, 8);

    if (health <= 0) {
        health = 0;
        state = EnemyState::DEAD;
        deathTimer = 0.0f;
        // Death particles
        particles->EmitBlood(bloodPos, {0, 1, 0}, 15);
    } else {
        // If was idle/patrol, switch to chase
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
        // Death animation - fall over and fade
        float t = deathTimer / 3.0f;
        alpha = 1.0f - t;
        yOffset = -t * (height * 0.5f);
    }

    Color body = bodyColor;
    if (hurtTimer > 0) body = WHITE;  // Flash white when hurt
    body.a = (unsigned char)(alpha * 255);

    float drawY = yOffset;

    // Body (capsule-like shape)
    Vector3 bodyPos = {position.x, drawY + height * 0.4f, position.z};
    DrawCapsule(
        {position.x, drawY + radius, position.z},
        {position.x, drawY + height - radius * 0.5f, position.z},
        radius, 4, 4, body
    );

    // Eyes
    if (IsAlive()) {
        float eyeY = drawY + height * 0.75f;
        float fwd_x = sinf(yaw * DEG2RAD) * (radius + 0.05f);
        float fwd_z = cosf(yaw * DEG2RAD) * (radius + 0.05f);
        float right_x = cosf(yaw * DEG2RAD) * 0.12f;
        float right_z = -sinf(yaw * DEG2RAD) * 0.12f;

        Color eye = eyeColor;
        eye.a = (unsigned char)(alpha * 255);

        DrawSphere({position.x + fwd_x + right_x, eyeY, position.z + fwd_z + right_z},
                   0.06f, eye);
        DrawSphere({position.x + fwd_x - right_x, eyeY, position.z + fwd_z - right_z},
                   0.06f, eye);
    }

    // Health bar (above head, only if damaged and alive)
    if (IsAlive() && health < maxHealth) {
        float barWidth = 0.8f;
        float healthFrac = (float)health / maxHealth;
        Vector3 barPos = {position.x, drawY + height + 0.3f, position.z};

        // Background
        DrawCube(barPos, barWidth, 0.08f, 0.08f, DARKGRAY);
        // Health
        float hWidth = barWidth * healthFrac;
        Color hColor = healthFrac > 0.5f ? GREEN : (healthFrac > 0.25f ? YELLOW : RED);
        DrawCube({barPos.x - (barWidth - hWidth) * 0.5f, barPos.y, barPos.z},
                 hWidth, 0.08f, 0.08f, hColor);
    }
}

bool Enemy::CanSeePlayer(Player* player, Level* level) const {
    float dist = Vector3Distance(position, player->position);
    if (dist > sightRange) return false;

    // Raycast check
    Vector3 start = position;
    start.y = height * 0.7f;
    Vector3 end = player->position;
    end.y = player->currentHeight;

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
