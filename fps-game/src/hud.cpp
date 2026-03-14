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

    char buf[128];
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

    // Wave info (top center)
    snprintf(buf, sizeof(buf), "WAVE: %d / %d",
             game->waves.currentWave + 1, game->waves.maxWaves);
    int waveW = MeasureText(buf, 22);
    DrawText(buf, sw / 2 - waveW / 2, 35, 22, YELLOW);

    if (game->waves.waveActive) {
        snprintf(buf, sizeof(buf), "ZOMBIES: %d", game->waves.enemiesRemaining);
    } else if (!game->waves.allWavesComplete) {
        snprintf(buf, sizeof(buf), "NEXT WAVE: %.0f", game->waves.waveTimer);
    } else {
        snprintf(buf, sizeof(buf), "ALL WAVES COMPLETE!");
    }
    int infoW = MeasureText(buf, 18);
    DrawText(buf, sw / 2 - infoW / 2, 60, 18, LIGHTGRAY);

    // Kills
    snprintf(buf, sizeof(buf), "KILLS: %d", game->kills);
    DrawText(buf, sw / 2 - 50, 82, 16, WHITE);

    // Co-op partner info
    if (game->coopMode != CoopMode::SOLO && game->remoteVis.connected) {
        int pX = 20;
        int pY = 30;
        DrawRectangle(pX - 2, pY - 2, 170, 50, {20, 20, 40, 180});

        DrawText("PARTNER", pX + 2, pY, 12, SKYBLUE);

        // Partner health bar
        float pHealth = (float)game->remoteVis.health / 100.0f;
        if (pHealth < 0) pHealth = 0;
        int pBarW = 150;
        int pBarH = 14;
        DrawRectangle(pX, pY + 16, pBarW, pBarH, {40, 40, 40, 200});
        Color pHColor = pHealth > 0.5f ? GREEN : (pHealth > 0.25f ? YELLOW : RED);
        DrawRectangle(pX, pY + 16, (int)(pBarW * pHealth), pBarH, pHColor);
        DrawRectangleLines(pX, pY + 16, pBarW, pBarH, {100, 100, 255, 200});

        snprintf(buf, sizeof(buf), "HP: %d", game->remoteVis.health);
        DrawText(buf, pX + 3, pY + 17, 12, WHITE);

        // Partner kills
        snprintf(buf, sizeof(buf), "Partner Kills: %d", game->partnerKills);
        DrawText(buf, pX, pY + 34, 10, LIGHTGRAY);

        // Ping
        snprintf(buf, sizeof(buf), "Ping: %.0fms", game->net.ping);
        DrawText(buf, sw - 100, 30, 12, {100, 100, 100, 255});
    }

    if (game->coopMode != CoopMode::SOLO) {
        DrawText("CO-OP", sw - 60, 15, 14, SKYBLUE);
    }

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
    int mmY = 50;
    int mmScale = 4;
    DrawRectangle(mmX - 2, mmY - 2, mmSize + 4, mmSize + 4, {0, 0, 0, 180});

    // Player dot on minimap
    DrawCircle(mmX + mmSize / 2, mmY + mmSize / 2, 3, GREEN);
    float dirX = sinf(player->yaw * DEG2RAD) * 8;
    float dirZ = cosf(player->yaw * DEG2RAD) * 8;
    DrawLine(mmX + mmSize / 2, mmY + mmSize / 2,
             mmX + mmSize / 2 + (int)dirX, mmY + mmSize / 2 + (int)dirZ, GREEN);

    // Partner dot on minimap
    if (game->coopMode != CoopMode::SOLO && game->remoteVis.connected) {
        float dx = (game->remoteVis.position.x - player->position.x) / game->level->tileSize;
        float dz = (game->remoteVis.position.z - player->position.z) / game->level->tileSize;
        int px = mmX + mmSize / 2 + (int)(dx * mmScale);
        int py = mmY + mmSize / 2 + (int)(dz * mmScale);
        if (px >= mmX && px < mmX + mmSize && py >= mmY && py < mmY + mmSize) {
            DrawCircle(px, py, 3, SKYBLUE);
        }
    }
}

void HUD::DrawMenu(Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, {10, 10, 15, 255});

    float t = GetTime();
    for (int i = 0; i < 20; i++) {
        float y = fmodf(i * 50.0f + t * 30.0f, (float)sh);
        DrawLine(0, (int)y, sw, (int)y, {30, 30, 40, 100});
    }

    // Title
    const char* title = "DARK ARENA";
    int titleW = MeasureText(title, 80);
    DrawText(title, sw / 2 - titleW / 2 + 3, sh / 5 + 3, 80, {100, 0, 0, 255});
    DrawText(title, sw / 2 - titleW / 2, sh / 5, 80, RED);

    const char* sub = "CO-OP ZOMBIE SURVIVAL";
    int subW = MeasureText(sub, 24);
    DrawText(sub, sw / 2 - subW / 2, sh / 5 + 90, 24, {200, 100, 50, 255});

    // Menu options
    const char* options[] = {"SOLO SURVIVAL", "HOST GAME", "JOIN GAME"};
    const char* descriptions[] = {
        "Survive 10 waves of zombies alone",
        "Host a 2-player co-op game",
        "Join a friend's game by IP"
    };

    for (int i = 0; i < 3; i++) {
        int optY = sh / 2 + i * 65 - 30;
        int optW = MeasureText(options[i], 30);

        bool selected = (game->menuSelection == i);

        if (selected) {
            DrawRectangle(sw / 2 - 180, optY - 8, 360, 52, {40, 40, 60, 200});
            DrawRectangleLines(sw / 2 - 180, optY - 8, 360, 52, {80, 80, 150, 255});
        }

        Color textColor = selected ? WHITE : GRAY;
        DrawText(options[i], sw / 2 - optW / 2, optY, 30, textColor);

        int descW = MeasureText(descriptions[i], 14);
        DrawText(descriptions[i], sw / 2 - descW / 2, optY + 34, 14,
                 selected ? LIGHTGRAY : DARKGRAY);

        if (selected) {
            float pulse = (sinf(t * 4.0f) + 1.0f) * 0.5f;
            DrawText(">", sw / 2 - 180 + 10, optY + 2, 28,
                     Fade(YELLOW, 0.5f + pulse * 0.5f));
        }
    }

    // Controls
    int ctrlY = sh - 80;
    DrawText("W/S or UP/DOWN to select    ENTER to confirm", sw / 2 - 210, ctrlY, 16, GRAY);
    DrawText("WASD - Move    MOUSE - Look    LMB - Shoot    1/2/3 - Weapons",
             sw / 2 - 270, ctrlY + 25, 14, {60, 60, 60, 255});

    DrawText("v2.0 | Co-Op | Powered by raylib", 10, sh - 25, 14, {60, 60, 60, 255});
}

void HUD::DrawLobby(Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, {10, 10, 20, 255});

    const char* title = game->coopMode == CoopMode::HOST ? "HOSTING GAME" : "JOIN GAME";
    int titleW = MeasureText(title, 50);
    DrawText(title, sw / 2 - titleW / 2, sh / 5, 50, SKYBLUE);

    if (game->coopMode == CoopMode::HOST) {
        // Host lobby
        if (!game->net.IsConnected()) {
            const char* waiting = "Waiting for player to connect...";
            int ww = MeasureText(waiting, 24);
            float pulse = (sinf(GetTime() * 2.0f) + 1.0f) * 0.5f;
            DrawText(waiting, sw / 2 - ww / 2, sh / 2 - 40, 24,
                     Fade(WHITE, 0.5f + pulse * 0.5f));

            char portText[64];
            snprintf(portText, sizeof(portText), "Port: %d", game->net.port);
            int pw = MeasureText(portText, 20);
            DrawText(portText, sw / 2 - pw / 2, sh / 2 + 10, 20, LIGHTGRAY);

            DrawText("Share your IP address with your partner",
                     sw / 2 - 180, sh / 2 + 50, 16, GRAY);
        } else {
            DrawText("PLAYER 2 CONNECTED!", sw / 2 - 130, sh / 2 - 40, 28, GREEN);

            float pulse = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
            const char* start = "Press ENTER to start!";
            int startW = MeasureText(start, 24);
            DrawText(start, sw / 2 - startW / 2, sh / 2 + 20, 24,
                     Fade(YELLOW, 0.5f + pulse * 0.5f));
        }
    } else {
        // Client lobby
        if (game->net.netState == NetState::DISCONNECTED) {
            DrawText("Enter host IP address:", sw / 2 - 120, sh / 2 - 60, 20, WHITE);

            // IP input box
            int boxW = 300;
            int boxH = 40;
            int boxX = sw / 2 - boxW / 2;
            int boxY = sh / 2 - 15;
            DrawRectangle(boxX, boxY, boxW, boxH, {30, 30, 40, 255});
            DrawRectangleLines(boxX, boxY, boxW, boxH, SKYBLUE);

            DrawText(game->net.ipInputBuffer, boxX + 10, boxY + 10, 22, WHITE);

            // Blinking cursor
            if ((int)(GetTime() * 2) % 2 == 0) {
                int cursorX = boxX + 10 + MeasureText(game->net.ipInputBuffer, 22);
                DrawRectangle(cursorX, boxY + 8, 2, 24, WHITE);
            }

            DrawText("Press ENTER to connect", sw / 2 - 100, boxY + 50, 16, GRAY);
        } else if (game->net.netState == NetState::CONNECTING) {
            float pulse = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
            DrawText("Connecting...", sw / 2 - 70, sh / 2, 24,
                     Fade(WHITE, 0.5f + pulse * 0.5f));
        } else if (game->net.IsConnected()) {
            DrawText("CONNECTED!", sw / 2 - 70, sh / 2 - 20, 28, GREEN);
            DrawText("Waiting for host to start...", sw / 2 - 130, sh / 2 + 20, 18, LIGHTGRAY);
        }
    }

    DrawText("Press ESC to go back", sw / 2 - 90, sh - 60, 16, GRAY);
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

    const char* text = "OVERRUN BY ZOMBIES";
    int w = MeasureText(text, 60);
    DrawText(text, sw / 2 - w / 2, sh / 4, 60, RED);

    char buf[128];
    snprintf(buf, sizeof(buf), "Survived to Wave %d", game->waves.currentWave + 1);
    int bw = MeasureText(buf, 28);
    DrawText(buf, sw / 2 - bw / 2, sh / 2 - 40, 28, WHITE);

    snprintf(buf, sizeof(buf), "Your Kills: %d", game->kills);
    bw = MeasureText(buf, 22);
    DrawText(buf, sw / 2 - bw / 2, sh / 2 + 10, 22, LIGHTGRAY);

    if (game->coopMode != CoopMode::SOLO) {
        snprintf(buf, sizeof(buf), "Partner Kills: %d    Total: %d",
                game->partnerKills, game->kills + game->partnerKills);
        bw = MeasureText(buf, 20);
        DrawText(buf, sw / 2 - bw / 2, sh / 2 + 40, 20, LIGHTGRAY);
    }

    float pulse = (sinf(GetTime() * 3.0f) + 1.0f) * 0.5f;
    const char* retry = game->coopMode == CoopMode::SOLO ?
                        "PRESS ENTER TO RETRY" : "PRESS ENTER FOR MENU";
    int rw = MeasureText(retry, 24);
    DrawText(retry, sw / 2 - rw / 2, sh * 3 / 4, 24,
             Fade(WHITE, 0.5f + pulse * 0.5f));
}

void HUD::DrawWin(Game* game) {
    int sw = GetScreenWidth();
    int sh = GetScreenHeight();

    DrawRectangle(0, 0, sw, sh, {0, 10, 20, 200});

    const char* text = "SURVIVED!";
    int w = MeasureText(text, 70);
    DrawText(text, sw / 2 - w / 2, sh / 4, 70, GOLD);

    const char* sub = "You survived all 10 waves!";
    int sw2 = MeasureText(sub, 24);
    DrawText(sub, sw / 2 - sw2 / 2, sh / 2 - 40, 24, WHITE);

    char buf[128];
    snprintf(buf, sizeof(buf), "Your Kills: %d", game->kills);
    int bw = MeasureText(buf, 24);
    DrawText(buf, sw / 2 - bw / 2, sh / 2, 24, LIGHTGRAY);

    if (game->coopMode != CoopMode::SOLO) {
        snprintf(buf, sizeof(buf), "Partner Kills: %d    Total: %d",
                game->partnerKills, game->kills + game->partnerKills);
        bw = MeasureText(buf, 20);
        DrawText(buf, sw / 2 - bw / 2, sh / 2 + 30, 20, LIGHTGRAY);
    }

    const char* cont = "PRESS ENTER FOR MENU";
    int cw = MeasureText(cont, 24);
    DrawText(cont, sw / 2 - cw / 2, sh * 3 / 4, 24, WHITE);
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
