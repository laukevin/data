#include "hud.h"
#include "player.h"
#include "game.h"
#include "weapon.h"
#include <cstdio>
#include <cmath>

void HUD::Init() {
    messageTimer = 0;
    message = nullptr;
    hitMarkerTimer = 0;
    screenShake = 0;
    damageVignette = 0;
}

void HUD::Update(float dt) {
    if (messageTimer > 0) messageTimer -= dt;
    if (hitMarkerTimer > 0) hitMarkerTimer -= dt;
    if (screenShake > 0) screenShake *= 0.9f;
    if (screenShake < 0.01f) screenShake = 0;
    if (damageVignette > 0) damageVignette -= dt * 2.0f;
}

void HUD::DrawGameHUD(Player* player, Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Crosshair
    if (game->showCrosshair) {
        int cx = sw / 2;
        int cy = sh / 2;
        Color crossColor = WHITE;
        int gap = 6;
        int len = 12;
        int thick = 2;

        DrawRectangle(cx - len - gap, cy - thick/2, len, thick, crossColor);
        DrawRectangle(cx + gap, cy - thick/2, len, thick, crossColor);
        DrawRectangle(cx - thick/2, cy - len - gap, thick, len, crossColor);
        DrawRectangle(cx - thick/2, cy + gap, thick, len, crossColor);

        // Dot
        DrawRectangle(cx - 1, cy - 1, 2, 2, crossColor);
    }

    // Hit marker
    if (hitMarkerTimer > 0) {
        int cx = sw / 2;
        int cy = sh / 2;
        Color hitColor = Fade(WHITE, hitMarkerTimer * 5.0f);
        int s = 10;
        DrawLine(cx - s, cy - s, cx - s/3, cy - s/3, hitColor);
        DrawLine(cx + s, cy - s, cx + s/3, cy - s/3, hitColor);
        DrawLine(cx - s, cy + s, cx - s/3, cy + s/3, hitColor);
        DrawLine(cx + s, cy + s, cx + s/3, cy + s/3, hitColor);
    }

    // Health bar
    int barW = 200;
    int barH = 20;
    int barX = 20;
    int barY = sh - 50;
    float healthPct = (float)player->health / player->maxHealth;

    DrawRectangle(barX - 1, barY - 1, barW + 2, barH + 2, {40, 40, 40, 200});
    Color healthColor = healthPct > 0.5f ? GREEN :
                        (healthPct > 0.25f ? YELLOW : RED);
    DrawRectangle(barX, barY, (int)(barW * healthPct), barH, healthColor);
    DrawRectangleLines(barX, barY, barW, barH, WHITE);

    char buf[64];
    snprintf(buf, sizeof(buf), "HP: %d", player->health);
    DrawText(buf, barX + 5, barY + 2, 16, WHITE);

    // Armor bar
    if (player->armor > 0) {
        int aBarY = barY - 28;
        float armorPct = (float)player->armor / player->maxArmor;
        DrawRectangle(barX - 1, aBarY - 1, barW + 2, barH + 2, {40, 40, 40, 200});
        DrawRectangle(barX, aBarY, (int)(barW * armorPct), barH, {50, 120, 255, 255});
        DrawRectangleLines(barX, aBarY, barW, barH, WHITE);
        snprintf(buf, sizeof(buf), "ARMOR: %d", player->armor);
        DrawText(buf, barX + 5, aBarY + 2, 16, WHITE);
    }

    // Weapon & Ammo
    WeaponDef& w = g_weapons.weapons[player->currentWeapon];
    int ammoX = sw - 220;
    int ammoY = sh - 50;

    DrawRectangle(ammoX - 5, ammoY - 25, 220, 70, {20, 20, 20, 180});
    DrawText(w.name, ammoX, ammoY - 22, 16, LIGHTGRAY);

    snprintf(buf, sizeof(buf), "%d", player->ammo[w.ammoIndex]);
    DrawText(buf, ammoX + 10, ammoY, 30, YELLOW);

    // Weapon slots
    for (int i = 0; i < WeaponSystem::MAX_WEAPONS; i++) {
        int slotX = ammoX + i * 60;
        int slotY = ammoY - 50;
        Color slotColor = {60, 60, 60, 180};
        if (i == player->currentWeapon) slotColor = {100, 100, 40, 220};
        if (!g_weapons.owned[i]) slotColor = {30, 30, 30, 100};

        DrawRectangle(slotX, slotY, 50, 20, slotColor);
        DrawRectangleLines(slotX, slotY, 50, 20, GRAY);
        snprintf(buf, sizeof(buf), "%d-%s", i + 1,
                 i == 0 ? "PIS" : (i == 1 ? "SHG" : "RIF"));
        DrawText(buf, slotX + 3, slotY + 3, 10,
                 g_weapons.owned[i] ? WHITE : DARKGRAY);
    }

    // Score & kills
    snprintf(buf, sizeof(buf), "KILLS: %d", game->kills);
    DrawText(buf, sw / 2 - 50, 40, 20, WHITE);

    snprintf(buf, sizeof(buf), "LEVEL: %d", game->currentLevel + 1);
    DrawText(buf, sw / 2 - 40, 65, 16, LIGHTGRAY);

    // Message
    if (messageTimer > 0 && message) {
        int msgW = MeasureText(message, 24);
        float alpha = messageTimer > 0.5f ? 1.0f : messageTimer * 2.0f;
        DrawText(message, sw / 2 - msgW / 2, sh / 2 - 100,
                 24, Fade(YELLOW, alpha));
    }

    // Damage vignette
    if (damageVignette > 0) {
        DrawRectangle(0, 0, sw, (int)(sh * 0.1f * damageVignette),
                     Fade(RED, damageVignette * 0.3f));
        DrawRectangle(0, sh - (int)(sh * 0.1f * damageVignette), sw,
                     (int)(sh * 0.1f * damageVignette),
                     Fade(RED, damageVignette * 0.3f));
    }

    // Low health warning
    if (player->health <= 25 && player->IsAlive()) {
        float pulse = (sinf(GetTime() * 8.0f) + 1.0f) * 0.5f;
        DrawRectangle(0, 0, sw, sh, Fade(RED, pulse * 0.15f));
    }

    // Minimap
    int mmSize = 150;
    int mmX = sw - mmSize - 15;
    int mmY = 15;
    int mmScale = 4;
    DrawRectangle(mmX - 2, mmY - 2, mmSize + 4, mmSize + 4, {0, 0, 0, 180});

    int playerGX = (int)(player->position.x / 4.0f);
    int playerGZ = (int)(player->position.z / 4.0f);

    int halfView = mmSize / (mmScale * 2);
    for (int dz = -halfView; dz <= halfView; dz++) {
        for (int dx = -halfView; dx <= halfView; dx++) {
            int gx = playerGX + dx;
            int gz = playerGZ + dz;
            if (gx < 0 || gx >= 32 || gz < 0 || gz >= 32) continue;

            int px = mmX + mmSize / 2 + dx * mmScale;
            int py = mmY + mmSize / 2 + dz * mmScale;
            if (px < mmX || px >= mmX + mmSize || py < mmY || py >= mmY + mmSize) continue;

            // Simplified minimap - just show explored area indicator
        }
    }

    // Player dot on minimap
    DrawCircle(mmX + mmSize / 2, mmY + mmSize / 2, 3, GREEN);
    // Direction indicator
    float dirX = sinf(player->yaw * DEG2RAD) * 8;
    float dirZ = cosf(player->yaw * DEG2RAD) * 8;
    DrawLine(mmX + mmSize / 2, mmY + mmSize / 2,
             mmX + mmSize / 2 + (int)dirX, mmY + mmSize / 2 + (int)dirZ, GREEN);
}

void HUD::DrawMenu(Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    // Background
    DrawRectangle(0, 0, sw, sh, {10, 10, 15, 255});

    // Animated background lines
    float t = GetTime();
    for (int i = 0; i < 20; i++) {
        float y = fmodf(i * 50.0f + t * 30.0f, (float)sh);
        DrawLine(0, (int)y, sw, (int)y, {30, 30, 40, 100});
    }

    // Title
    const char* title = "DARK ARENA";
    int titleW = MeasureText(title, 80);
    DrawText(title, sw / 2 - titleW / 2 + 3, sh / 4 + 3, 80, {100, 0, 0, 255});
    DrawText(title, sw / 2 - titleW / 2, sh / 4, 80, RED);

    // Subtitle
    const char* sub = "A First-Person Shooter";
    int subW = MeasureText(sub, 24);
    DrawText(sub, sw / 2 - subW / 2, sh / 4 + 90, 24, LIGHTGRAY);

    // Start prompt
    float pulse = (sinf(t * 3.0f) + 1.0f) * 0.5f;
    const char* start = "PRESS ENTER OR SPACE TO START";
    int startW = MeasureText(start, 24);
    DrawText(start, sw / 2 - startW / 2, sh * 2 / 3, 24,
             Fade(WHITE, 0.5f + pulse * 0.5f));

    // Controls
    int ctrlY = sh * 2 / 3 + 60;
    DrawText("WASD - Move    MOUSE - Look    LMB - Shoot", sw / 2 - 220, ctrlY, 16, GRAY);
    DrawText("SHIFT - Sprint    SPACE - Jump    1/2/3 - Weapons", sw / 2 - 240, ctrlY + 25, 16, GRAY);
    DrawText("ESC - Pause    SCROLL - Switch Weapon", sw / 2 - 200, ctrlY + 50, 16, GRAY);

    // Version
    DrawText("v1.0 | Powered by raylib", 10, sh - 25, 14, {60, 60, 60, 255});
}

void HUD::DrawPaused() {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, {0, 0, 0, 150});

    const char* text = "PAUSED";
    int w = MeasureText(text, 60);
    DrawText(text, sw / 2 - w / 2, sh / 2 - 60, 60, WHITE);

    const char* resume = "Press ESC to Resume";
    int rw = MeasureText(resume, 20);
    DrawText(resume, sw / 2 - rw / 2, sh / 2 + 20, 20, LIGHTGRAY);

    const char* quit = "Press Q to Quit to Menu";
    int qw = MeasureText(quit, 20);
    DrawText(quit, sw / 2 - qw / 2, sh / 2 + 50, 20, GRAY);
}

void HUD::DrawGameOver(Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, {20, 0, 0, 200});

    const char* text = "YOU DIED";
    int w = MeasureText(text, 70);
    DrawText(text, sw / 2 - w / 2, sh / 3, 70, RED);

    char buf[64];
    snprintf(buf, sizeof(buf), "Kills: %d    Level: %d", game->kills, game->currentLevel + 1);
    int bw = MeasureText(buf, 24);
    DrawText(buf, sw / 2 - bw / 2, sh / 2, 24, LIGHTGRAY);

    float pulse = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
    const char* retry = "PRESS ENTER TO RETRY";
    int rw = MeasureText(retry, 24);
    DrawText(retry, sw / 2 - rw / 2, sh * 2 / 3, 24,
             Fade(WHITE, 0.5f + pulse * 0.5f));
}

void HUD::DrawWin(Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, {0, 10, 20, 200});

    const char* text = "VICTORY!";
    int w = MeasureText(text, 70);
    DrawText(text, sw / 2 - w / 2, sh / 3, 70, GOLD);

    const char* sub = "You cleared all 5 levels!";
    int sw2 = MeasureText(sub, 24);
    DrawText(sub, sw / 2 - sw2 / 2, sh / 2 - 20, 24, WHITE);

    char buf[64];
    snprintf(buf, sizeof(buf), "Total Kills: %d", game->kills);
    int bw = MeasureText(buf, 24);
    DrawText(buf, sw / 2 - bw / 2, sh / 2 + 20, 24, LIGHTGRAY);

    const char* cont = "PRESS ENTER FOR MENU";
    int cw = MeasureText(cont, 24);
    DrawText(cont, sw / 2 - cw / 2, sh * 2 / 3, 24, WHITE);
}

void HUD::ShowMessage(const char* msg, float duration) {
    message = msg;
    messageTimer = duration;
}

void HUD::TriggerHitMarker() {
    hitMarkerTimer = 0.2f;
}

void HUD::TriggerScreenShake(float amount) {
    screenShake = amount;
}

void HUD::TriggerDamageEffect() {
    damageVignette = 1.0f;
}
