#include "game.h"
#include "player.h"
#include "level.h"
#include "enemy.h"
#include "weapon.h"
#include "particles.h"
#include "hud.h"
#include "audio_manager.h"
#include "renderer.h"
#include <cstdio>

void Game::Init() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "DARK ARENA - FPS");
    SetTargetFPS(60);
    InitAudioDevice();
    DisableCursor();

    player = std::make_unique<Player>();
    level = std::make_unique<Level>();
    particles = std::make_unique<ParticleSystem>();
    hud = std::make_unique<HUD>();
    audio = std::make_unique<AudioManager>();
    renderer = std::make_unique<Renderer>();

    particles->Init();
    hud->Init();
    audio->Init();
    renderer->Init(screenWidth, screenHeight);
    g_weapons.Init();

    state = GameState::MENU;
}

void Game::StartGame() {
    currentLevel = 0;
    score = 0;
    kills = 0;
    enemies.clear();

    level->Generate(currentLevel);
    player->Init(level->spawnPoint);
    g_weapons = WeaponSystem();
    g_weapons.Init();
    SpawnEnemies();

    state = GameState::PLAYING;
    gameTime = 0.0f;
    hud->ShowMessage("LEVEL 1 - CLEAR ALL ENEMIES", 3.0f);
}

void Game::NextLevel() {
    currentLevel++;
    if (currentLevel >= 5) {
        state = GameState::WIN;
        EnableCursor();
        return;
    }
    enemies.clear();
    level->Unload();
    level->Generate(currentLevel);
    player->Init(level->spawnPoint);
    // Keep weapons and ammo
    SpawnEnemies();
    char msg[64];
    snprintf(msg, sizeof(msg), "LEVEL %d - CLEAR ALL ENEMIES", currentLevel + 1);
    hud->ShowMessage(msg, 3.0f);
}

void Game::RestartGame() {
    enemies.clear();
    level->Unload();
    StartGame();
}

void Game::SpawnEnemies() {
    int baseCount = 5 + currentLevel * 3;

    for (int i = 0; i < baseCount; i++) {
        // Find a valid spawn position
        Vector3 pos = {0};
        bool valid = false;
        for (int attempt = 0; attempt < 100; attempt++) {
            int gx = GetRandomValue(1, Level::MAP_SIZE - 2);
            int gz = GetRandomValue(1, Level::MAP_SIZE - 2);
            if (!level->IsWallTile(gx, gz)) {
                pos = {(float)gx * level->tileSize + level->tileSize * 0.5f,
                       0.0f,
                       (float)gz * level->tileSize + level->tileSize * 0.5f};
                float dist = Vector3Distance(pos, player->position);
                if (dist > 10.0f) {
                    valid = true;
                    break;
                }
            }
        }
        if (!valid) continue;

        Enemy e;
        // Mix enemy types based on level
        int roll = GetRandomValue(0, 100);
        if (currentLevel >= 3 && roll < 15) {
            e.Init(EnemyType::HEAVY, pos);
        } else if (currentLevel >= 1 && roll < 40) {
            e.Init(EnemyType::SOLDIER, pos);
        } else if (roll < 60) {
            e.Init(EnemyType::DEMON, pos);
        } else {
            e.Init(EnemyType::GRUNT, pos);
        }
        enemies.push_back(e);
    }
}

void Game::Update() {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;  // Cap delta time

    audio->Update();

    switch (state) {
        case GameState::MENU:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                StartGame();
                DisableCursor();
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                // Will trigger WindowShouldClose
            }
            break;

        case GameState::PLAYING: {
            gameTime += dt;

            // Pause
            if (IsKeyPressed(KEY_ESCAPE)) {
                state = GameState::PAUSED;
                EnableCursor();
                break;
            }

            // Player update
            player->Update(dt, level.get());

            // Weapon update
            g_weapons.Update(dt, player.get(), enemies, level.get(), particles.get());

            // Weapon switching
            if (IsKeyPressed(KEY_ONE)) g_weapons.SwitchWeapon(0, player.get());
            if (IsKeyPressed(KEY_TWO)) g_weapons.SwitchWeapon(1, player.get());
            if (IsKeyPressed(KEY_THREE)) g_weapons.SwitchWeapon(2, player.get());
            float wheel = GetMouseWheelMove();
            if (wheel != 0) {
                int dir = (wheel > 0) ? -1 : 1;
                int next = player->currentWeapon;
                for (int i = 0; i < (int)WeaponType::COUNT; i++) {
                    next = (next + dir + (int)WeaponType::COUNT) % (int)WeaponType::COUNT;
                    if (g_weapons.owned[next]) {
                        g_weapons.SwitchWeapon(next, player.get());
                        break;
                    }
                }
            }

            // Enemy update
            int aliveCount = 0;
            for (auto& e : enemies) {
                if (e.active) {
                    e.Update(dt, player.get(), level.get(), particles.get());
                    if (e.IsAlive()) aliveCount++;
                }
            }

            // Check pickups
            for (auto& p : level->pickups) {
                if (!p.active) continue;
                float dist = Vector3Distance(player->position, p.position);
                if (dist < 1.5f) {
                    bool picked = false;
                    switch (p.type) {
                        case 0: // health
                            if (player->health < player->maxHealth) {
                                player->AddHealth(25);
                                picked = true;
                                hud->ShowMessage("+25 HEALTH", 1.0f);
                            }
                            break;
                        case 1: // armor
                            if (player->armor < player->maxArmor) {
                                player->AddArmor(25);
                                picked = true;
                                hud->ShowMessage("+25 ARMOR", 1.0f);
                            }
                            break;
                        case 2: // ammo pistol
                            player->ammo[0] += 15;
                            picked = true;
                            hud->ShowMessage("+15 PISTOL AMMO", 1.0f);
                            break;
                        case 3: // ammo shotgun
                            player->ammo[1] += 8;
                            picked = true;
                            hud->ShowMessage("+8 SHOTGUN SHELLS", 1.0f);
                            break;
                        case 4: // ammo rifle
                            player->ammo[2] += 30;
                            picked = true;
                            hud->ShowMessage("+30 RIFLE AMMO", 1.0f);
                            break;
                        case 5: // weapon shotgun
                            if (!g_weapons.owned[1]) {
                                g_weapons.owned[1] = true;
                                player->ammo[1] += 8;
                                picked = true;
                                hud->ShowMessage("GOT SHOTGUN!", 2.0f);
                                g_weapons.SwitchWeapon(1, player.get());
                            } else {
                                player->ammo[1] += 4;
                                picked = true;
                            }
                            break;
                        case 6: // weapon rifle
                            if (!g_weapons.owned[2]) {
                                g_weapons.owned[2] = true;
                                player->ammo[2] += 30;
                                picked = true;
                                hud->ShowMessage("GOT ASSAULT RIFLE!", 2.0f);
                                g_weapons.SwitchWeapon(2, player.get());
                            } else {
                                player->ammo[2] += 15;
                                picked = true;
                            }
                            break;
                    }
                    if (picked) {
                        p.active = false;
                        audio->PlayPickup();
                        particles->EmitPickup(p.position, GREEN);
                    }
                }
            }

            // Level exit check
            if (aliveCount == 0 && !level->exitUnlocked) {
                level->exitUnlocked = true;
                hud->ShowMessage("ALL ENEMIES CLEARED! FIND THE EXIT!", 3.0f);
            }
            if (level->exitUnlocked) {
                float exitDist = Vector3Distance(player->position, level->exitPoint);
                if (exitDist < 2.0f) {
                    NextLevel();
                }
            }

            // Check death
            if (!player->IsAlive()) {
                state = GameState::GAME_OVER;
                EnableCursor();
            }

            // Particles
            particles->Update(dt);

            // HUD
            hud->Update(dt);
            break;
        }

        case GameState::PAUSED:
            if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_P)) {
                state = GameState::PLAYING;
                DisableCursor();
            }
            if (IsKeyPressed(KEY_Q)) {
                state = GameState::MENU;
            }
            break;

        case GameState::GAME_OVER:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                RestartGame();
                DisableCursor();
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                state = GameState::MENU;
            }
            break;

        case GameState::WIN:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                state = GameState::MENU;
            }
            break;
    }
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(BLACK);

    if (state == GameState::PLAYING || state == GameState::PAUSED) {
        // Apply screen shake
        Camera3D cam = player->camera;
        if (hud->screenShake > 0.01f) {
            cam.target.x += (float)GetRandomValue(-100, 100) / 100.0f * hud->screenShake;
            cam.target.y += (float)GetRandomValue(-100, 100) / 100.0f * hud->screenShake;
        }

        BeginMode3D(cam);

        // Draw level
        level->Draw(cam);
        level->DrawPickups(gameTime);

        // Draw enemies
        for (auto& e : enemies) {
            if (e.active) e.Draw();
        }

        // Draw projectiles
        g_weapons.DrawProjectiles();

        // Draw particles
        particles->Draw(cam);

        // Draw exit marker
        if (level->exitUnlocked) {
            Vector3 ep = level->exitPoint;
            ep.y = 1.0f + sinf(gameTime * 3.0f) * 0.3f;
            DrawCube(ep, 1.5f, 2.5f, 1.5f, Fade(GREEN, 0.3f));
            DrawCubeWires(ep, 1.5f, 2.5f, 1.5f, GREEN);
        }

        EndMode3D();

        // Draw weapon model (2D overlay)
        g_weapons.DrawWeaponModel(player.get());

        // Damage vignette
        if (player->damageFlash > 0) {
            DrawRectangle(0, 0, screenWidth, screenHeight,
                         Fade(RED, player->damageFlash * 0.3f));
        }

        // Draw HUD
        hud->DrawGameHUD(player.get(), this);

        if (state == GameState::PAUSED) {
            hud->DrawPaused();
        }
    } else if (state == GameState::MENU) {
        hud->DrawMenu(this);
    } else if (state == GameState::GAME_OVER) {
        hud->DrawGameOver(this);
    } else if (state == GameState::WIN) {
        hud->DrawWin(this);
    }

    DrawFPS(10, 10);
    EndDrawing();
}

void Game::Shutdown() {
    level->Unload();
    audio->Shutdown();
    renderer->Shutdown();
    CloseAudioDevice();
    CloseWindow();
}
