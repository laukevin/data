#include "game.h"
#include "player.h"
#include "level.h"
#include "enemy.h"
#include "weapon.h"
#include "particles.h"
#include "hud.h"
#include "audio_manager.h"
#include "renderer.h"
#include "network.h"
#include <cstdio>
#include <cstdlib>

void Game::Init() {
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(screenWidth, screenHeight, "DARK ARENA - CO-OP ZOMBIE SURVIVAL");
    SetTargetFPS(60);
    InitAudioDevice();

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
    net.Init();

    state = GameState::MENU;
    EnableCursor();
}

void Game::GoToMenu() {
    net.Disconnect();
    coopMode = CoopMode::SOLO;
    state = GameState::MENU;
    menuSelection = 0;
    EnableCursor();
}

void Game::StartGame() {
    // Solo mode
    coopMode = CoopMode::SOLO;
    currentLevel = 0;
    score = 0;
    kills = 0;
    partnerKills = 0;
    enemies.clear();

    levelSeed = GetRandomValue(1, 999999);
    SetRandomSeed(levelSeed);
    level->Generate(currentLevel);
    player->Init(level->spawnPoint);
    g_weapons = WeaponSystem();
    g_weapons.Init();

    // Wave system
    waves = WaveSystem();
    waves.currentWave = 0;
    waves.waveActive = false;
    waves.waveTimer = 3.0f;
    waves.allWavesComplete = false;

    state = GameState::PLAYING;
    gameTime = 0.0f;
    DisableCursor();
    hud->ShowMessage("WAVE 1 INCOMING...", 3.0f);
}

void Game::StartCoopGame() {
    currentLevel = 0;
    score = 0;
    kills = 0;
    partnerKills = 0;
    enemies.clear();

    if (net.IsHost()) {
        levelSeed = GetRandomValue(1, 999999);
        SetRandomSeed(levelSeed);
        level->Generate(currentLevel);
        player->Init(level->spawnPoint);

        // Send game start to client
        net.SendGameStart(levelSeed, currentLevel);
    }
    // Client waits for GAME_START packet to set seed

    g_weapons = WeaponSystem();
    g_weapons.Init();

    waves = WaveSystem();
    waves.currentWave = 0;
    waves.waveActive = false;
    waves.waveTimer = 5.0f;
    waves.allWavesComplete = false;

    net.netState = NetState::IN_GAME;

    state = GameState::PLAYING;
    gameTime = 0.0f;
    DisableCursor();
    hud->ShowMessage("CO-OP: SURVIVE THE ZOMBIE HORDE!", 3.0f);
}

void Game::NextWave() {
    waves.currentWave++;
    if (waves.currentWave >= waves.maxWaves) {
        waves.allWavesComplete = true;
        if (coopMode == CoopMode::SOLO || net.IsHost()) {
            state = GameState::WIN;
            EnableCursor();
            if (net.IsConnected()) net.SendGameWin();
        }
        return;
    }

    int count = waves.GetEnemyCount(waves.currentWave);

    // In co-op, more zombies
    if (coopMode != CoopMode::SOLO) {
        count = (int)(count * 1.5f);
    }

    SpawnWaveEnemies(count);
    waves.enemiesThisWave = count;
    waves.enemiesRemaining = count;
    waves.waveActive = true;

    char msg[64];
    snprintf(msg, sizeof(msg), "WAVE %d - %d ZOMBIES!", waves.currentWave + 1, count);
    hud->ShowMessage(msg, 3.0f);

    if (net.IsHost() && net.IsConnected()) {
        net.SendWaveStart(waves.currentWave, count);
    }
}

void Game::RestartGame() {
    enemies.clear();
    level->Unload();
    if (coopMode == CoopMode::SOLO) {
        StartGame();
    } else {
        StartCoopGame();
    }
}

void Game::SpawnWaveEnemies(int count) {
    for (int i = 0; i < count; i++) {
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
                if (dist > 12.0f) {
                    valid = true;
                    break;
                }
            }
        }
        if (!valid) continue;

        Enemy e;
        int roll = GetRandomValue(0, 100);
        int wave = waves.currentWave;

        if (wave >= 7 && roll < 15) {
            e.Init(EnemyType::ZOMBIE_BRUTE, pos);
        } else if (wave >= 3 && roll < 35) {
            e.Init(EnemyType::ZOMBIE_SPITTER, pos);
        } else if (roll < 55) {
            e.Init(EnemyType::ZOMBIE_RUNNER, pos);
        } else {
            e.Init(EnemyType::ZOMBIE_WALKER, pos);
        }
        enemies.push_back(e);
    }
}

// Legacy - not used in wave mode anymore
void Game::SpawnEnemies() {
    SpawnWaveEnemies(waves.GetEnemyCount(waves.currentWave));
    waves.enemiesThisWave = waves.GetEnemyCount(waves.currentWave);
    waves.enemiesRemaining = waves.enemiesThisWave;
    waves.waveActive = true;
}

void Game::SendLocalPlayerState() {
    if (!net.IsConnected()) return;
    if (net.sendTimer < net.sendRate) return;
    net.sendTimer = 0;

    NetPlayerState nps;
    nps.position = player->position;
    nps.yaw = player->yaw;
    nps.pitch = player->pitch;
    nps.health = player->health;
    nps.armor = player->armor;
    nps.currentWeapon = player->currentWeapon;
    nps.isSprinting = player->isSprinting;
    nps.isCrouching = player->isCrouching;
    nps.isShooting = false;
    net.SendPlayerState(nps);
}

void Game::ProcessNetPackets() {
    while (net.HasPendingPackets()) {
        NetPacket pkt = net.PopPacket();

        switch (pkt.type) {
            case NetMsg::PLAYER_STATE: {
                NetPlayerState nps = pkt.ReadPlayerState(0);
                remoteVis.position = nps.position;
                remoteVis.yaw = nps.yaw;
                remoteVis.pitch = nps.pitch;
                remoteVis.health = nps.health;
                remoteVis.armor = nps.armor;
                remoteVis.currentWeapon = nps.currentWeapon;
                remoteVis.isSprinting = nps.isSprinting;
                remoteVis.isCrouching = nps.isCrouching;
                remoteVis.connected = true;
                net.remotePlayer = nps;
                break;
            }

            case NetMsg::PLAYER_SHOOT: {
                Vector3 origin = {pkt.ReadFloat(0), pkt.ReadFloat(4), pkt.ReadFloat(8)};
                Vector3 dir = {pkt.ReadFloat(12), pkt.ReadFloat(16), pkt.ReadFloat(20)};
                int weaponIdx = pkt.ReadInt(24);
                HandleRemoteShoot(origin, dir, weaponIdx);
                break;
            }

            case NetMsg::ENEMY_DAMAGED: {
                int idx = pkt.ReadInt(0);
                int dmg = pkt.ReadInt(4);
                Vector3 hitDir = {pkt.ReadFloat(8), pkt.ReadFloat(12), pkt.ReadFloat(16)};
                if (idx >= 0 && idx < (int)enemies.size()) {
                    enemies[idx].TakeDamage(dmg, hitDir, particles.get());
                }
                break;
            }

            case NetMsg::ENEMY_KILLED: {
                int idx = pkt.ReadInt(0);
                int scorer = pkt.ReadInt(4);
                if (idx >= 0 && idx < (int)enemies.size()) {
                    if (scorer == 1) partnerKills++;
                    else kills++;
                }
                break;
            }

            case NetMsg::ENEMY_STATES: {
                // Client receives authoritative enemy states from host
                if (!net.IsHost()) {
                    int count = pkt.ReadShort(0);
                    int startIdx = pkt.ReadShort(2);
                    int offset = 4;
                    for (int i = 0; i < count; i++) {
                        NetEnemyState nes;
                        memcpy(&nes, pkt.data + offset, sizeof(NetEnemyState));
                        offset += sizeof(NetEnemyState);

                        int idx = startIdx + i;
                        // Grow vector if needed
                        while (idx >= (int)enemies.size()) {
                            Enemy e;
                            e.active = false;
                            enemies.push_back(e);
                        }
                        enemies[idx].position = nes.position;
                        enemies[idx].yaw = nes.yaw;
                        enemies[idx].health = nes.health;
                        enemies[idx].state = (EnemyState)nes.state;
                        enemies[idx].type = (EnemyType)nes.type;
                        enemies[idx].active = nes.active;
                    }
                }
                break;
            }

            case NetMsg::PICKUP_UPDATE: {
                int idx = pkt.ReadInt(0);
                if (idx >= 0 && idx < (int)level->pickups.size()) {
                    level->pickups[idx].active = false;
                }
                break;
            }

            case NetMsg::PLAYER_DAMAGED: {
                // Remote player took damage (for HUD display)
                int playerId = pkt.ReadInt(0);
                int amount = pkt.ReadInt(4);
                if (playerId == 0) {
                    // We got damaged by host telling us
                    player->TakeDamage(amount);
                }
                break;
            }

            case NetMsg::GAME_START: {
                // Client receives game seed
                levelSeed = pkt.ReadInt(0);
                currentLevel = pkt.ReadInt(4);
                SetRandomSeed(levelSeed);
                level->Generate(currentLevel);
                // Spawn player at slightly offset position
                Vector3 spawn = level->spawnPoint;
                spawn.x += 2.0f;
                player->Init(spawn);
                break;
            }

            case NetMsg::WAVE_START: {
                int waveNum = pkt.ReadInt(0);
                int enemyCount = pkt.ReadInt(4);
                waves.currentWave = waveNum;
                waves.enemiesThisWave = enemyCount;
                waves.enemiesRemaining = enemyCount;
                waves.waveActive = true;
                char msg[64];
                snprintf(msg, sizeof(msg), "WAVE %d - %d ZOMBIES!", waveNum + 1, enemyCount);
                hud->ShowMessage(msg, 3.0f);
                break;
            }

            case NetMsg::LEVEL_COMPLETE: {
                hud->ShowMessage("WAVE CLEARED!", 2.0f);
                break;
            }

            case NetMsg::GAME_OVER: {
                state = GameState::GAME_OVER;
                EnableCursor();
                break;
            }

            case NetMsg::GAME_WIN: {
                state = GameState::WIN;
                EnableCursor();
                break;
            }

            case NetMsg::CONNECT_ACCEPT: {
                // Client got accepted by host
                net.remoteConnected = true;
                net.netState = NetState::CONNECTED;
                break;
            }

            case NetMsg::DISCONNECT: {
                remoteVis.connected = false;
                net.remoteConnected = false;
                hud->ShowMessage("PARTNER DISCONNECTED", 3.0f);
                break;
            }

            default:
                break;
        }
    }
}

void Game::HandleRemoteShoot(Vector3 origin, Vector3 dir, int weaponIdx) {
    if (weaponIdx < 0 || weaponIdx >= WeaponSystem::MAX_WEAPONS) return;

    WeaponDef& w = g_weapons.weapons[weaponIdx];
    remoteVis.shootFlash = 1.0f;

    // Perform raycast for remote player's shot
    for (int p = 0; p < w.pellets; p++) {
        float spreadX = (GetRandomValue(-100, 100) / 100.0f) * w.spread * DEG2RAD;
        float spreadY = (GetRandomValue(-100, 100) / 100.0f) * w.spread * DEG2RAD;

        Vector3 shotDir = dir;
        Vector3 right = Vector3CrossProduct(shotDir, {0, 1, 0});
        right = Vector3Normalize(right);
        Vector3 up = Vector3CrossProduct(right, shotDir);
        shotDir = Vector3Add(shotDir, Vector3Scale(right, spreadX));
        shotDir = Vector3Add(shotDir, Vector3Scale(up, spreadY));
        shotDir = Vector3Normalize(shotDir);

        // Only host does authoritative hit detection
        if (net.IsHost()) {
            // Raycast against enemies
            float closestDist = w.range;
            int hitIdx = -1;
            Vector3 hitPoint = {0};

            for (int i = 0; i < (int)enemies.size(); i++) {
                if (!enemies[i].active || !enemies[i].IsAlive()) continue;

                Vector3 ePos = enemies[i].position;
                float eR = enemies[i].radius;
                float eH = enemies[i].height;

                float tMin = 0.0f, tMax = w.range;
                bool hit = true;
                for (int axis = 0; axis < 3 && hit; axis++) {
                    float oA, dA, eA, rA;
                    if (axis == 0) { oA = origin.x; dA = shotDir.x; eA = ePos.x; rA = eR; }
                    else if (axis == 1) { oA = origin.y; dA = shotDir.y; eA = ePos.y + eH*0.5f; rA = eH*0.5f; }
                    else { oA = origin.z; dA = shotDir.z; eA = ePos.z; rA = eR; }

                    if (fabsf(dA) < 0.0001f) {
                        if (oA < eA - rA || oA > eA + rA) hit = false;
                    } else {
                        float t1 = (eA - rA - oA) / dA;
                        float t2 = (eA + rA - oA) / dA;
                        if (t1 > t2) { float tmp = t1; t1 = t2; t2 = tmp; }
                        tMin = tMin > t1 ? tMin : t1;
                        tMax = tMax < t2 ? tMax : t2;
                        if (tMin > tMax) hit = false;
                    }
                }
                if (hit && tMin >= 0 && tMin < closestDist) {
                    closestDist = tMin;
                    hitIdx = i;
                    hitPoint = Vector3Add(origin, Vector3Scale(shotDir, tMin));
                }
            }

            // Check wall
            float step = 0.3f;
            for (float t = 0; t < closestDist; t += step) {
                Vector3 pt = Vector3Add(origin, Vector3Scale(shotDir, t));
                if (level->IsWall(pt.x, pt.z)) {
                    closestDist = t;
                    hitIdx = -1;
                    hitPoint = Vector3Add(origin, Vector3Scale(shotDir, t - step*0.5f));
                    break;
                }
            }

            if (hitIdx >= 0) {
                Vector3 hitDir = Vector3Negate(shotDir);
                enemies[hitIdx].TakeDamage(w.damage, hitDir, particles.get());
                particles->EmitBlood(hitPoint, hitDir, 5);

                if (!enemies[hitIdx].IsAlive()) {
                    partnerKills++;
                    net.SendEnemyKill(hitIdx, 1);
                }
            } else if (closestDist < w.range) {
                particles->EmitSpark(hitPoint, {0, 1, 0}, 3);
            }
        }
    }

    // Visual: muzzle flash at remote player position
    particles->EmitMuzzleFlash(origin, dir);
}

void Game::UpdateNetwork(float dt) {
    net.Update(dt);
    ProcessNetPackets();

    if (state == GameState::PLAYING && net.IsConnected()) {
        SendLocalPlayerState();

        // Host sends enemy states periodically
        if (net.IsHost() && net.sendTimer <= 0) {
            net.SendEnemyStates(enemies);
        }
    }
}

void Game::Update() {
    float dt = GetFrameTime();
    if (dt > 0.05f) dt = 0.05f;

    audio->Update();

    // Network update (always, even in menu/lobby)
    if (coopMode != CoopMode::SOLO) {
        UpdateNetwork(dt);
    }

    switch (state) {
        case GameState::MENU: {
            // Menu navigation
            if (IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W)) {
                menuSelection--;
                if (menuSelection < 0) menuSelection = 2;
            }
            if (IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S)) {
                menuSelection++;
                if (menuSelection > 2) menuSelection = 0;
            }
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                switch (menuSelection) {
                    case 0: // Solo
                        StartGame();
                        break;
                    case 1: // Host
                        coopMode = CoopMode::HOST;
                        net.Init();
                        if (net.StartHost()) {
                            state = GameState::LOBBY;
                        } else {
                            hud->ShowMessage("FAILED TO START SERVER", 2.0f);
                            coopMode = CoopMode::SOLO;
                        }
                        break;
                    case 2: // Join
                        coopMode = CoopMode::CLIENT;
                        net.Init();
                        state = GameState::LOBBY;
                        break;
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                // Exit handled by WindowShouldClose
            }
            break;
        }

        case GameState::LOBBY: {
            if (IsKeyPressed(KEY_ESCAPE)) {
                GoToMenu();
                break;
            }

            if (coopMode == CoopMode::CLIENT && net.netState == NetState::DISCONNECTED) {
                // IP input
                int key = GetCharPressed();
                while (key > 0) {
                    if ((key >= '0' && key <= '9') || key == '.') {
                        if (net.ipInputLen < 63) {
                            net.ipInputBuffer[net.ipInputLen] = (char)key;
                            net.ipInputLen++;
                            net.ipInputBuffer[net.ipInputLen] = '\0';
                        }
                    }
                    key = GetCharPressed();
                }
                if (IsKeyPressed(KEY_BACKSPACE) && net.ipInputLen > 0) {
                    net.ipInputLen--;
                    net.ipInputBuffer[net.ipInputLen] = '\0';
                }
                if (IsKeyPressed(KEY_ENTER)) {
                    if (!net.ConnectToHost(net.ipInputBuffer)) {
                        hud->ShowMessage("CONNECTION FAILED", 2.0f);
                    }
                }
            }

            // Host: start game when client connected
            if (coopMode == CoopMode::HOST && net.IsConnected()) {
                if (IsKeyPressed(KEY_ENTER)) {
                    StartCoopGame();
                }
            }

            // Client: auto-start when we receive GAME_START
            if (coopMode == CoopMode::CLIENT && net.netState == NetState::IN_GAME) {
                // Already handled in ProcessNetPackets -> GAME_START
                if (state != GameState::PLAYING) {
                    state = GameState::PLAYING;
                    DisableCursor();
                    hud->ShowMessage("CO-OP: SURVIVE THE ZOMBIE HORDE!", 3.0f);
                }
            }
            break;
        }

        case GameState::PLAYING: {
            gameTime += dt;

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

            // Send shoot events to network
            if (coopMode != CoopMode::SOLO && net.IsConnected()) {
                WeaponDef& w = g_weapons.weapons[player->currentWeapon];
                bool wantFire = w.automatic ? IsMouseButtonDown(MOUSE_BUTTON_LEFT) :
                                               IsMouseButtonPressed(MOUSE_BUTTON_LEFT);
                if (wantFire && g_weapons.fireCooldown <= 0 &&
                    player->ammo[w.ammoIndex] >= w.ammoPerShot) {
                    Vector3 origin = player->camera.position;
                    Vector3 baseDir = Vector3Normalize(
                        Vector3Subtract(player->camera.target, player->camera.position));
                    net.SendShoot(origin, baseDir, player->currentWeapon);
                }
            }

            // Enemy update (host is authoritative, or solo)
            bool isAuthority = (coopMode == CoopMode::SOLO || net.IsHost());
            Vector3* remotePos = nullptr;
            bool remoteAlive = false;
            if (remoteVis.connected && remoteVis.health > 0) {
                remotePos = &remoteVis.position;
                remoteAlive = true;
            }

            int aliveCount = 0;
            for (auto& e : enemies) {
                if (e.active) {
                    if (isAuthority) {
                        e.Update(dt, player.get(), level.get(), particles.get(),
                                remotePos, remoteAlive);
                    }
                    if (e.IsAlive()) aliveCount++;
                }
            }
            waves.enemiesRemaining = aliveCount;

            // Remote player interpolation
            if (remoteVis.connected) {
                remoteVis.Interpolate(dt);
                if (remoteVis.shootFlash > 0) remoteVis.shootFlash -= dt * 5.0f;
            }

            // Wave system
            if (isAuthority) {
                if (!waves.waveActive && !waves.allWavesComplete) {
                    waves.waveTimer -= dt;
                    if (waves.waveTimer <= 0) {
                        NextWave();
                    }
                }
                if (waves.waveActive && aliveCount == 0) {
                    waves.waveActive = false;
                    waves.waveTimer = waves.waveCooldown;

                    if (waves.currentWave + 1 >= waves.maxWaves) {
                        waves.allWavesComplete = true;
                        state = GameState::WIN;
                        EnableCursor();
                        if (net.IsConnected()) net.SendGameWin();
                    } else {
                        char msg[64];
                        snprintf(msg, sizeof(msg), "WAVE %d CLEARED! NEXT WAVE IN 5...",
                                waves.currentWave + 1);
                        hud->ShowMessage(msg, 3.0f);
                        if (net.IsConnected()) net.SendLevelComplete();
                    }
                }
            }

            // Check pickups (both players)
            for (int pi = 0; pi < (int)level->pickups.size(); pi++) {
                auto& p = level->pickups[pi];
                if (!p.active) continue;
                float dist = Vector3Distance(player->position, p.position);
                if (dist < 1.5f) {
                    bool picked = false;
                    switch (p.type) {
                        case 0:
                            if (player->health < player->maxHealth) {
                                player->AddHealth(25);
                                picked = true;
                                hud->ShowMessage("+25 HEALTH", 1.0f);
                            }
                            break;
                        case 1:
                            if (player->armor < player->maxArmor) {
                                player->AddArmor(25);
                                picked = true;
                                hud->ShowMessage("+25 ARMOR", 1.0f);
                            }
                            break;
                        case 2:
                            player->ammo[0] += 15;
                            picked = true;
                            hud->ShowMessage("+15 PISTOL AMMO", 1.0f);
                            break;
                        case 3:
                            player->ammo[1] += 8;
                            picked = true;
                            hud->ShowMessage("+8 SHOTGUN SHELLS", 1.0f);
                            break;
                        case 4:
                            player->ammo[2] += 30;
                            picked = true;
                            hud->ShowMessage("+30 RIFLE AMMO", 1.0f);
                            break;
                        case 5:
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
                        case 6:
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
                        if (net.IsConnected()) net.SendPickupCollected(pi);
                    }
                }
            }

            // Check death
            if (!player->IsAlive()) {
                // In co-op, game over only if both dead
                if (coopMode == CoopMode::SOLO ||
                    !remoteVis.connected || remoteVis.health <= 0) {
                    state = GameState::GAME_OVER;
                    EnableCursor();
                    if (net.IsConnected()) net.SendGameOver();
                } else {
                    // Partner still alive, spectate-ish
                    hud->ShowMessage("YOU DIED! Partner still fighting...", 2.0f);
                }
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
                GoToMenu();
            }
            break;

        case GameState::GAME_OVER:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                if (coopMode == CoopMode::SOLO) {
                    RestartGame();
                    DisableCursor();
                } else {
                    GoToMenu();
                }
            }
            if (IsKeyPressed(KEY_ESCAPE)) {
                GoToMenu();
            }
            break;

        case GameState::WIN:
            if (IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE)) {
                GoToMenu();
            }
            break;
    }
}

void Game::Draw() {
    BeginDrawing();
    ClearBackground(BLACK);

    if (state == GameState::PLAYING || state == GameState::PAUSED) {
        Camera3D cam = player->camera;
        if (hud->screenShake > 0.01f) {
            cam.target.x += (float)GetRandomValue(-100, 100) / 100.0f * hud->screenShake;
            cam.target.y += (float)GetRandomValue(-100, 100) / 100.0f * hud->screenShake;
        }

        BeginMode3D(cam);

        level->Draw(cam);
        level->DrawPickups(gameTime);

        // Draw enemies
        for (auto& e : enemies) {
            if (e.active) e.Draw();
        }

        // Draw remote player
        if (remoteVis.connected) {
            remoteVis.Draw();
        }

        g_weapons.DrawProjectiles();
        particles->Draw(cam);

        EndMode3D();

        // Draw weapon model (2D overlay)
        g_weapons.DrawWeaponModel(player.get());

        // Damage vignette
        if (player->damageFlash > 0) {
            DrawRectangle(0, 0, screenWidth, screenHeight,
                         Fade(RED, player->damageFlash * 0.3f));
        }

        hud->DrawGameHUD(player.get(), this);

        if (state == GameState::PAUSED) {
            hud->DrawPaused();
        }
    } else if (state == GameState::MENU) {
        hud->DrawMenu(this);
    } else if (state == GameState::LOBBY) {
        hud->DrawLobby(this);
    } else if (state == GameState::GAME_OVER) {
        hud->DrawGameOver(this);
    } else if (state == GameState::WIN) {
        hud->DrawWin(this);
    }

    DrawFPS(10, 10);
    EndDrawing();
}

void Game::Shutdown() {
    net.Shutdown();
    level->Unload();
    audio->Shutdown();
    renderer->Shutdown();
    CloseAudioDevice();
    CloseWindow();
}

// ---- RemotePlayerVisual ----

void RemotePlayerVisual::Interpolate(float dt) {
    float lerpSpeed = 15.0f;
    displayPos.x += (position.x - displayPos.x) * lerpSpeed * dt;
    displayPos.y += (position.y - displayPos.y) * lerpSpeed * dt;
    displayPos.z += (position.z - displayPos.z) * lerpSpeed * dt;
    displayYaw += (yaw - displayYaw) * lerpSpeed * dt;
}

void RemotePlayerVisual::Draw() {
    if (!connected) return;

    float h = isCrouching ? 1.0f : 1.7f;
    Color bodyCol = {40, 120, 200, 255};   // Blue teammate
    Color helmetCol = {50, 140, 220, 255};

    // Body capsule
    DrawCapsule(
        {displayPos.x, displayPos.y + 0.3f, displayPos.z},
        {displayPos.x, displayPos.y + h - 0.2f, displayPos.z},
        0.35f, 4, 4, bodyCol
    );

    // Helmet
    DrawSphere({displayPos.x, displayPos.y + h, displayPos.z}, 0.25f, helmetCol);

    // Eyes direction
    float fwd_x = sinf(displayYaw * DEG2RAD) * 0.3f;
    float fwd_z = cosf(displayYaw * DEG2RAD) * 0.3f;
    DrawSphere({displayPos.x + fwd_x, displayPos.y + h, displayPos.z + fwd_z},
               0.08f, WHITE);

    // Shoot flash
    if (shootFlash > 0.3f) {
        Vector3 muzzle = {displayPos.x + fwd_x * 2.5f, displayPos.y + h - 0.2f,
                          displayPos.z + fwd_z * 2.5f};
        DrawSphere(muzzle, 0.15f, Fade(YELLOW, shootFlash));
    }

    // Name tag
    // Health bar above head
    float barWidth = 0.8f;
    float healthFrac = (float)health / 100.0f;
    if (healthFrac < 0) healthFrac = 0;
    Vector3 barPos = {displayPos.x, displayPos.y + h + 0.5f, displayPos.z};
    DrawCube(barPos, barWidth, 0.06f, 0.06f, {40, 40, 40, 200});
    Color hColor = healthFrac > 0.5f ? GREEN : (healthFrac > 0.25f ? YELLOW : RED);
    float hWidth = barWidth * healthFrac;
    DrawCube({barPos.x - (barWidth - hWidth) * 0.5f, barPos.y, barPos.z},
             hWidth, 0.06f, 0.06f, hColor);

    // "ALLY" text indicator
    DrawCube({displayPos.x, displayPos.y + h + 0.7f, displayPos.z},
             0.1f, 0.1f, 0.1f, SKYBLUE);
}
