#include "weapon.h"
#include "player.h"
#include "enemy.h"
#include "level.h"
#include "particles.h"
#include "rlgl.h"
#include <cmath>
#include <algorithm>

WeaponSystem g_weapons;

void WeaponSystem::Init() {
    weapons[0] = {WeaponType::PISTOL, "PISTOL", 15, 3.0f, 1, 0, 1.5f, 1, 100.0f, 2.0f, false, YELLOW};
    weapons[1] = {WeaponType::SHOTGUN, "SHOTGUN", 8, 1.2f, 1, 1, 5.0f, 8, 30.0f, 5.0f, false, ORANGE};
    weapons[2] = {WeaponType::RIFLE, "RIFLE", 10, 8.0f, 1, 2, 2.5f, 1, 80.0f, 1.5f, true, YELLOW};

    owned[0] = true;
    owned[1] = false;
    owned[2] = false;
    fireCooldown = 0;
    muzzleFlash = 0;
    recoilOffset = 0;
    swayTimer = 0;
    switchTimer = 0;
    switchTo = -1;
    projectiles.clear();
}

void WeaponSystem::Update(float dt, Player* player, std::vector<Enemy>& enemies,
                           Level* level, ParticleSystem* particles) {
    fireCooldown -= dt;
    muzzleFlash -= dt * 10.0f;
    if (muzzleFlash < 0) muzzleFlash = 0;
    recoilOffset *= 0.85f;
    swayTimer += dt;

    // Weapon switch animation
    if (switchTimer > 0) {
        switchTimer -= dt * 5.0f;
        if (switchTimer <= 0 && switchTo >= 0) {
            player->currentWeapon = switchTo;
            switchTo = -1;
            switchTimer = 0.3f;  // Raise animation
        }
    }

    // Fire
    WeaponDef& w = weapons[player->currentWeapon];
    bool wantFire = w.automatic ? IsMouseButtonDown(MOUSE_BUTTON_LEFT) :
                                   IsMouseButtonPressed(MOUSE_BUTTON_LEFT);

    if (wantFire && fireCooldown <= 0 && switchTimer <= 0) {
        if (player->ammo[w.ammoIndex] >= w.ammoPerShot) {
            Fire(player, enemies, level, particles);
        }
    }

    // Update projectiles
    for (auto& p : projectiles) {
        if (!p.active) continue;
        p.position = Vector3Add(p.position, Vector3Scale(p.velocity, dt));
        p.lifetime -= dt;
        if (p.lifetime <= 0) {
            p.active = false;
            continue;
        }
        // Trail
        particles->EmitTrail(p.position, p.color);

        // Check wall collision
        if (level->IsWall(p.position.x, p.position.z)) {
            particles->EmitSpark(p.position, Vector3Normalize(p.velocity), 5);
            p.active = false;
        }
    }
}

void WeaponSystem::Fire(Player* player, std::vector<Enemy>& enemies,
                         Level* level, ParticleSystem* particles) {
    WeaponDef& w = weapons[player->currentWeapon];
    player->ammo[w.ammoIndex] -= w.ammoPerShot;
    fireCooldown = 1.0f / w.fireRate;
    muzzleFlash = 1.0f;
    recoilOffset = w.kickback;

    Vector3 origin = player->camera.position;
    Vector3 baseDir = Vector3Normalize(Vector3Subtract(player->camera.target, player->camera.position));

    for (int i = 0; i < w.pellets; i++) {
        // Add spread
        float spreadX = (GetRandomValue(-100, 100) / 100.0f) * w.spread * DEG2RAD;
        float spreadY = (GetRandomValue(-100, 100) / 100.0f) * w.spread * DEG2RAD;

        Vector3 dir = baseDir;
        // Rotate by spread
        Vector3 right = Vector3CrossProduct(dir, {0, 1, 0});
        right = Vector3Normalize(right);
        Vector3 up = Vector3CrossProduct(right, dir);

        dir = Vector3Add(dir, Vector3Scale(right, spreadX));
        dir = Vector3Add(dir, Vector3Scale(up, spreadY));
        dir = Vector3Normalize(dir);

        Vector3 hitPoint;
        int hitEnemy;
        if (RaycastHit(origin, dir, w.range, enemies, level, hitPoint, hitEnemy)) {
            if (hitEnemy >= 0) {
                Vector3 hitDir = Vector3Negate(dir);
                enemies[hitEnemy].TakeDamage(w.damage, hitDir, particles);
                particles->EmitBlood(hitPoint, hitDir, 5);

                // Check if killed
                if (!enemies[hitEnemy].IsAlive()) {
                    // Score handled in game update
                }
            } else {
                // Hit wall
                Vector3 normal = {0, 0, 0};
                // Approximate normal
                float testDist = 0.1f;
                if (level->IsWall(hitPoint.x + testDist, hitPoint.z))
                    normal.x = -1;
                else if (level->IsWall(hitPoint.x - testDist, hitPoint.z))
                    normal.x = 1;
                else if (level->IsWall(hitPoint.x, hitPoint.z + testDist))
                    normal.z = -1;
                else if (level->IsWall(hitPoint.x, hitPoint.z - testDist))
                    normal.z = 1;
                else normal = {0, 1, 0};

                particles->EmitSpark(hitPoint, normal, 5);
            }
        }
    }

    // Muzzle flash particles
    Vector3 muzzlePos = Vector3Add(origin, Vector3Scale(baseDir, 1.0f));
    particles->EmitMuzzleFlash(muzzlePos, baseDir);
}

void WeaponSystem::DrawWeaponModel(Player* player) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();
    WeaponDef& w = weapons[player->currentWeapon];

    float sway = sinf(swayTimer * 2.0f) * 3.0f;
    float bobX = sinf(player->bobTimer * 0.5f) * 5.0f;
    float bobY = fabsf(cosf(player->bobTimer * 0.5f)) * 3.0f;

    float weaponY = sh - 180.0f + recoilOffset * 15.0f + bobY;
    float weaponX = sw * 0.6f + sway + bobX;

    // Switch animation offset
    if (switchTimer > 0.3f) {
        weaponY += (switchTimer - 0.3f) * 400.0f;
    } else if (switchTimer > 0) {
        weaponY += switchTimer * 200.0f;
    }

    // Weapon body
    Color gunColor = {60, 60, 65, 255};
    Color gunHighlight = {80, 80, 85, 255};

    switch (w.type) {
        case WeaponType::PISTOL:
            // Barrel
            DrawRectangle((int)weaponX, (int)weaponY, 20, 60, gunColor);
            DrawRectangle((int)weaponX + 2, (int)weaponY + 5, 16, 50, gunHighlight);
            // Grip
            DrawRectangle((int)weaponX - 5, (int)weaponY + 60, 30, 70, gunColor);
            DrawRectangle((int)weaponX - 3, (int)weaponY + 65, 26, 60, {50, 40, 35, 255});
            // Slide
            DrawRectangle((int)weaponX - 2, (int)weaponY, 24, 15, {70, 70, 75, 255});
            break;

        case WeaponType::SHOTGUN:
            weaponX -= 20;
            // Long barrel
            DrawRectangle((int)weaponX, (int)weaponY - 20, 16, 90, gunColor);
            DrawRectangle((int)weaponX + 16, (int)weaponY - 15, 12, 80, {55, 55, 60, 255});
            // Stock
            DrawRectangle((int)weaponX - 8, (int)weaponY + 70, 40, 80, {60, 45, 35, 255});
            // Pump
            DrawRectangle((int)weaponX - 3, (int)weaponY + 30, 22, 25, {70, 60, 50, 255});
            break;

        case WeaponType::RIFLE:
            weaponX -= 15;
            // Barrel
            DrawRectangle((int)weaponX, (int)weaponY - 30, 12, 100, gunColor);
            // Body
            DrawRectangle((int)weaponX - 10, (int)weaponY + 20, 35, 40, gunHighlight);
            // Magazine
            DrawRectangle((int)weaponX + 3, (int)weaponY + 60, 14, 40, {50, 50, 55, 255});
            // Stock
            DrawRectangle((int)weaponX - 15, (int)weaponY + 40, 50, 60, {55, 45, 35, 255});
            // Sight
            DrawRectangle((int)weaponX + 2, (int)weaponY - 35, 8, 10, RED);
            break;

        default:
            break;
    }

    // Muzzle flash
    if (muzzleFlash > 0.3f) {
        Color flashColor = w.muzzleColor;
        flashColor.a = (unsigned char)(muzzleFlash * 200);
        float flashSize = 20.0f + muzzleFlash * 30.0f;
        DrawCircle((int)(weaponX + 8), (int)(weaponY - 30 * (w.type == WeaponType::RIFLE ? 1.3f : 1.0f)),
                   (int)flashSize, flashColor);
        DrawCircle((int)(weaponX + 8), (int)(weaponY - 30 * (w.type == WeaponType::RIFLE ? 1.3f : 1.0f)),
                   (int)(flashSize * 0.5f), WHITE);
    }
}

void WeaponSystem::DrawProjectiles() {
    for (auto& p : projectiles) {
        if (!p.active) continue;
        DrawSphere(p.position, p.radius, p.color);
    }
}

void WeaponSystem::SwitchWeapon(int index, Player* player) {
    if (index < 0 || index >= MAX_WEAPONS) return;
    if (!owned[index]) return;
    if (index == player->currentWeapon && switchTo < 0) return;

    switchTo = index;
    switchTimer = 0.3f;  // Lower animation
}

bool WeaponSystem::RaycastHit(Vector3 origin, Vector3 dir, float maxDist,
                                std::vector<Enemy>& enemies, Level* level,
                                Vector3& hitPoint, int& hitEnemyIdx) {
    hitEnemyIdx = -1;
    float closestDist = maxDist;

    // Check enemies
    for (int i = 0; i < (int)enemies.size(); i++) {
        if (!enemies[i].active || !enemies[i].IsAlive()) continue;

        // Simple cylinder collision
        Vector3 ePos = enemies[i].position;
        float eR = enemies[i].radius;
        float eH = enemies[i].height;

        // Ray vs cylinder (simplified as ray vs vertical AABB)
        float tMin = 0.0f;
        float tMax = maxDist;

        // XZ plane check (circle approximated as square for speed)
        for (int axis = 0; axis < 3; axis++) {
            float oA, dA, eA, rA;
            if (axis == 0) { oA = origin.x; dA = dir.x; eA = ePos.x; rA = eR; }
            else if (axis == 1) { oA = origin.y; dA = dir.y; eA = ePos.y + eH * 0.5f; rA = eH * 0.5f; }
            else { oA = origin.z; dA = dir.z; eA = ePos.z; rA = eR; }

            if (fabsf(dA) < 0.0001f) {
                if (oA < eA - rA || oA > eA + rA) { tMin = maxDist + 1; break; }
            } else {
                float t1 = (eA - rA - oA) / dA;
                float t2 = (eA + rA - oA) / dA;
                if (t1 > t2) std::swap(t1, t2);
                tMin = std::max(tMin, t1);
                tMax = std::min(tMax, t2);
                if (tMin > tMax) break;
            }
        }

        if (tMin <= tMax && tMin >= 0 && tMin < closestDist) {
            closestDist = tMin;
            hitEnemyIdx = i;
            hitPoint = Vector3Add(origin, Vector3Scale(dir, tMin));
        }
    }

    // Check walls
    float step = 0.3f;
    for (float t = 0; t < closestDist; t += step) {
        Vector3 p = Vector3Add(origin, Vector3Scale(dir, t));
        if (level->IsWall(p.x, p.z)) {
            closestDist = t;
            hitPoint = p;
            hitEnemyIdx = -1;  // Wall hit overrides if closer
            // Back up slightly
            hitPoint = Vector3Add(origin, Vector3Scale(dir, t - step * 0.5f));
            break;
        }
    }

    return closestDist < maxDist;
}
