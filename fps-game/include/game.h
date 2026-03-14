#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>
#include <memory>

#include "player.h"
#include "level.h"
#include "enemy.h"
#include "weapon.h"
#include "particles.h"
#include "hud.h"
#include "audio_manager.h"
#include "renderer.h"

enum class GameState {
    MENU,
    PLAYING,
    PAUSED,
    GAME_OVER,
    WIN
};

struct Game {
    GameState state = GameState::MENU;
    int screenWidth = 1280;
    int screenHeight = 720;
    float gameTime = 0.0f;
    int currentLevel = 0;
    int score = 0;
    int kills = 0;
    bool showCrosshair = true;

    std::unique_ptr<Player> player;
    std::unique_ptr<Level> level;
    std::vector<Enemy> enemies;
    std::unique_ptr<ParticleSystem> particles;
    std::unique_ptr<HUD> hud;
    std::unique_ptr<AudioManager> audio;
    std::unique_ptr<Renderer> renderer;

    void Init();
    void Update();
    void Draw();
    void Shutdown();
    void StartGame();
    void NextLevel();
    void RestartGame();
    void SpawnEnemies();
};
