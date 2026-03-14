#include "player.h"
#include "level.h"
#include <cmath>

void Player::Init(Vector3 startPos) {
    position = startPos;
    position.y = 0.0f;
    velocity = {0, 0, 0};
    yaw = 0.0f;
    pitch = 0.0f;
    health = maxHealth;
    armor = 0;
    ammo[0] = 50;  // pistol
    ammo[1] = 0;
    ammo[2] = 0;
    ammo[3] = 0;
    currentWeapon = 0;
    isGrounded = false;
    isSprinting = false;
    isCrouching = false;
    currentHeight = playerHeight;
    damageFlash = 0.0f;
    bobTimer = 0.0f;

    camera.position = {position.x, currentHeight, position.z};
    camera.target = {position.x + 1.0f, currentHeight, position.z};
    camera.up = {0.0f, 1.0f, 0.0f};
    camera.fovy = 75.0f;
    camera.projection = CAMERA_PERSPECTIVE;
}

Vector3 Player::GetForward() const {
    float ry = yaw * DEG2RAD;
    float rp = pitch * DEG2RAD;
    return Vector3Normalize({cosf(rp) * sinf(ry), sinf(rp), cosf(rp) * cosf(ry)});
}

Vector3 Player::GetRight() const {
    float ry = yaw * DEG2RAD;
    return {cosf(ry), 0, -sinf(ry)};
}

void Player::Update(float dt, Level* level) {
    if (!IsAlive()) return;

    // Mouse look
    Vector2 mouseDelta = GetMouseDelta();
    float sensitivity = 0.15f;
    yaw += mouseDelta.x * sensitivity;
    pitch -= mouseDelta.y * sensitivity;
    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;

    // Sprint & Crouch
    isSprinting = IsKeyDown(KEY_LEFT_SHIFT) && !isCrouching;
    if (IsKeyPressed(KEY_LEFT_CONTROL) || IsKeyPressed(KEY_C)) {
        isCrouching = !isCrouching;
    }

    float targetHeight = isCrouching ? crouchHeight : playerHeight;
    currentHeight += (targetHeight - currentHeight) * 10.0f * dt;

    // Movement
    Vector3 forward = {sinf(yaw * DEG2RAD), 0.0f, cosf(yaw * DEG2RAD)};
    Vector3 right = {cosf(yaw * DEG2RAD), 0.0f, -sinf(yaw * DEG2RAD)};

    Vector3 moveDir = {0};
    if (IsKeyDown(KEY_W)) moveDir = Vector3Add(moveDir, forward);
    if (IsKeyDown(KEY_S)) moveDir = Vector3Subtract(moveDir, forward);
    if (IsKeyDown(KEY_D)) moveDir = Vector3Add(moveDir, right);
    if (IsKeyDown(KEY_A)) moveDir = Vector3Subtract(moveDir, right);

    if (Vector3Length(moveDir) > 0.01f) {
        moveDir = Vector3Normalize(moveDir);
    }

    float speed = moveSpeed;
    if (isSprinting) speed *= sprintMultiplier;
    if (isCrouching) speed *= 0.5f;

    velocity.x = moveDir.x * speed;
    velocity.z = moveDir.z * speed;

    // Gravity & Jump
    if (isGrounded && (IsKeyPressed(KEY_SPACE))) {
        velocity.y = jumpForce;
        isGrounded = false;
    }
    velocity.y += gravity * dt;

    // Apply movement with collision
    Vector3 newPos = position;
    newPos.x += velocity.x * dt;
    newPos.z += velocity.z * dt;
    newPos.y += velocity.y * dt;

    // Collision detection with level
    newPos = level->ResolveCollision(position, newPos, playerRadius);

    // Floor collision
    if (newPos.y < 0.0f) {
        newPos.y = 0.0f;
        velocity.y = 0.0f;
        isGrounded = true;
    }

    // Ceiling collision
    if (newPos.y + currentHeight > level->ceilingHeight - 0.1f) {
        newPos.y = level->ceilingHeight - currentHeight - 0.1f;
        velocity.y = 0.0f;
    }

    position = newPos;

    // Head bob
    bool isMoving = (fabsf(velocity.x) > 0.5f || fabsf(velocity.z) > 0.5f) && isGrounded;
    if (isMoving) {
        float bobSpeed = isSprinting ? 14.0f : 10.0f;
        bobTimer += dt * bobSpeed;
    } else {
        bobTimer = 0.0f;
    }

    float bob = isMoving ? sinf(bobTimer) * bobAmount * (isSprinting ? 1.5f : 1.0f) : 0.0f;

    // Update camera
    camera.position = {position.x, position.y + currentHeight + bob, position.z};
    Vector3 fwd = GetForward();
    camera.target = Vector3Add(camera.position, fwd);

    // Damage flash decay
    if (damageFlash > 0) damageFlash -= dt * 3.0f;
    if (damageFlash < 0) damageFlash = 0;
}

void Player::TakeDamage(int amount) {
    if (!IsAlive()) return;

    // Armor absorbs some damage
    if (armor > 0) {
        int absorbed = amount / 2;
        if (absorbed > armor) absorbed = armor;
        armor -= absorbed;
        amount -= absorbed;
    }

    health -= amount;
    if (health < 0) health = 0;
    damageFlash = 1.0f;
}

void Player::AddHealth(int amount) {
    health += amount;
    if (health > maxHealth) health = maxHealth;
}

void Player::AddArmor(int amount) {
    armor += amount;
    if (armor > maxArmor) armor = maxArmor;
}
