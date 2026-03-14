#pragma once
#include "raylib.h"

struct Player;
struct Game;

struct HUD {
    float messageTimer = 0.0f;
    const char* message = nullptr;
    float fadeAlpha = 0.0f;
    float hitMarkerTimer = 0.0f;

    // Screen effects
    float screenShake = 0.0f;
    float damageVignette = 0.0f;

    void Init();
    void Update(float dt);
    void DrawGameHUD(Player* player, Game* game);
    void DrawMenu(Game* game);
    void DrawLobby(Game* game);
    void DrawPaused();
    void DrawGameOver(Game* game);
    void DrawWin(Game* game);
    void ShowMessage(const char* msg, float duration = 2.0f);
    void TriggerHitMarker();
    void TriggerScreenShake(float amount);
    void TriggerDamageEffect();
};
