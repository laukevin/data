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
#include "network.h"

enum class GameState {
    MENU,
    LOBBY,
    PLAYING,
    PAUSED,
    GAME_OVER,
    WIN
};

enum class CoopMode {
    SOLO,
    HOST,
    CLIENT
};

struct RemotePlayerVisual {
    Vector3 position = {0};
    Vector3 displayPos = {0};  // Interpolated
    float yaw = 0.0f;
    float pitch = 0.0f;
    float displayYaw = 0.0f;
    int health = 100;
    int armor = 0;
    int currentWeapon = 0;
    bool isSprinting = false;
    bool isCrouching = false;
    bool connected = false;
    float shootFlash = 0.0f;

    void Draw();
    void Interpolate(float dt);
};

// Zombie wave system for co-op
struct WaveSystem {
    int currentWave = 0;
    int maxWaves = 10;
    int enemiesRemaining = 0;
    int enemiesThisWave = 0;
    float waveTimer = 0.0f;     // Countdown between waves
    float waveCooldown = 5.0f;  // Seconds between waves
    bool waveActive = false;
    bool allWavesComplete = false;

    int GetEnemyCount(int wave) const {
        return 6 + wave * 4;  // Scales up each wave
    }
};

struct Game {
    GameState state = GameState::MENU;
    int screenWidth = 1280;
    int screenHeight = 720;
    float gameTime = 0.0f;
    int currentLevel = 0;
    int score = 0;
    int kills = 0;
    int partnerKills = 0;
    bool showCrosshair = true;

    // Co-op
    CoopMode coopMode = CoopMode::SOLO;
    Network net;
    RemotePlayerVisual remoteVis;
    WaveSystem waves;
    int levelSeed = 0;

    // Menu selection
    int menuSelection = 0;  // 0=solo, 1=host, 2=join

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
    void StartCoopGame();
    void NextWave();
    void RestartGame();
    void SpawnEnemies();
    void SpawnWaveEnemies(int count);
    void UpdateNetwork(float dt);
    void ProcessNetPackets();
    void SendLocalPlayerState();
    void HandleRemoteShoot(Vector3 origin, Vector3 dir, int weaponIdx);
    void GoToMenu();
};
