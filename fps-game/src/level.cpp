#include "level.h"
#include "rlgl.h"
#include <cmath>
#include <cstring>
#include <algorithm>

struct Room {
    int x, y, w, h;
};

void Level::Generate(int index) {
    levelIndex = index;
    memset(map, 0, sizeof(map));

    // Fill all with walls
    for (int z = 0; z < MAP_SIZE; z++)
        for (int x = 0; x < MAP_SIZE; x++)
            map[z][x] = TileType::WALL;

    GenerateRooms();
    PlacePickups();
    PlaceLights();

    // Generate textures
    Color wallBase = {80, 75, 70, 255};
    Color floorBase = {50, 50, 55, 255};
    Color ceilBase = {40, 40, 45, 255};
    Color doorBase = {100, 60, 40, 255};

    // Vary colors per level
    switch (index % 4) {
        case 0: // Stone dungeon
            wallBase = {80, 75, 70, 255};
            floorBase = {50, 50, 55, 255};
            break;
        case 1: // Tech base
            wallBase = {60, 65, 80, 255};
            floorBase = {45, 50, 60, 255};
            break;
        case 2: // Hell
            wallBase = {90, 50, 40, 255};
            floorBase = {60, 35, 30, 255};
            break;
        case 3: // Dark fortress
            wallBase = {45, 45, 50, 255};
            floorBase = {35, 35, 40, 255};
            break;
    }

    wallTex = GenProceduralTexture(64, 64, wallBase, 0);
    floorTex = GenProceduralTexture(64, 64, floorBase, 1);
    ceilTex = GenProceduralTexture(64, 64, ceilBase, 2);
    doorTex = GenProceduralTexture(64, 64, doorBase, 3);

    BuildMeshes();
}

void Level::GenerateRooms() {
    std::vector<Room> rooms;
    int numRooms = 8 + levelIndex * 2;
    if (numRooms > 20) numRooms = 20;

    for (int i = 0; i < numRooms * 5; i++) {
        if ((int)rooms.size() >= numRooms) break;

        int w = GetRandomValue(3, 7);
        int h = GetRandomValue(3, 7);
        int x = GetRandomValue(1, MAP_SIZE - w - 2);
        int y = GetRandomValue(1, MAP_SIZE - h - 2);

        // Check overlap
        bool overlap = false;
        for (auto& r : rooms) {
            if (x < r.x + r.w + 1 && x + w + 1 > r.x &&
                y < r.y + r.h + 1 && y + h + 1 > r.y) {
                overlap = true;
                break;
            }
        }
        if (overlap) continue;

        // Carve room
        for (int rz = y; rz < y + h; rz++)
            for (int rx = x; rx < x + w; rx++)
                map[rz][rx] = TileType::EMPTY;

        // Add some pillars in large rooms
        if (w >= 5 && h >= 5) {
            map[y + 1][x + 1] = TileType::PILLAR;
            map[y + h - 2][x + 1] = TileType::PILLAR;
            map[y + 1][x + w - 2] = TileType::PILLAR;
            map[y + h - 2][x + w - 2] = TileType::PILLAR;
        }

        rooms.push_back({x, y, w, h});
    }

    // Connect rooms with corridors
    for (int i = 1; i < (int)rooms.size(); i++) {
        int cx1 = rooms[i-1].x + rooms[i-1].w / 2;
        int cy1 = rooms[i-1].y + rooms[i-1].h / 2;
        int cx2 = rooms[i].x + rooms[i].w / 2;
        int cy2 = rooms[i].y + rooms[i].h / 2;
        CarveCorridor(cx1, cy1, cx2, cy2);
    }

    // Set spawn and exit
    if (!rooms.empty()) {
        spawnPoint = {
            (rooms[0].x + rooms[0].w / 2.0f) * tileSize + tileSize * 0.5f,
            0.0f,
            (rooms[0].y + rooms[0].h / 2.0f) * tileSize + tileSize * 0.5f
        };
        auto& lastRoom = rooms.back();
        exitPoint = {
            (lastRoom.x + lastRoom.w / 2.0f) * tileSize + tileSize * 0.5f,
            0.0f,
            (lastRoom.y + lastRoom.h / 2.0f) * tileSize + tileSize * 0.5f
        };
    }
    exitUnlocked = false;
}

void Level::CarveCorridor(int x1, int y1, int x2, int y2) {
    int x = x1, y = y1;

    // L-shaped corridor
    while (x != x2) {
        if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE)
            if (map[y][x] == TileType::WALL) map[y][x] = TileType::EMPTY;
        // Also carve adjacent for wider corridors
        if (y + 1 < MAP_SIZE && map[y+1][x] == TileType::WALL) map[y+1][x] = TileType::EMPTY;
        x += (x2 > x1) ? 1 : -1;
    }
    while (y != y2) {
        if (x >= 0 && x < MAP_SIZE && y >= 0 && y < MAP_SIZE)
            if (map[y][x] == TileType::WALL) map[y][x] = TileType::EMPTY;
        if (x + 1 < MAP_SIZE && map[y][x+1] == TileType::WALL) map[y][x+1] = TileType::EMPTY;
        y += (y2 > y1) ? 1 : -1;
    }
}

void Level::PlacePickups() {
    pickups.clear();
    int count = 10 + levelIndex * 3;

    for (int i = 0; i < count; i++) {
        for (int attempt = 0; attempt < 50; attempt++) {
            int gx = GetRandomValue(1, MAP_SIZE - 2);
            int gz = GetRandomValue(1, MAP_SIZE - 2);
            if (!IsWallTile(gx, gz)) {
                Pickup p;
                p.position = {gx * tileSize + tileSize * 0.5f,
                              0.5f,
                              gz * tileSize + tileSize * 0.5f};
                p.bobOffset = GetRandomValue(0, 628) / 100.0f;

                int roll = GetRandomValue(0, 100);
                if (roll < 20) p.type = 0;       // health
                else if (roll < 35) p.type = 1;  // armor
                else if (roll < 55) p.type = 2;  // ammo pistol
                else if (roll < 70) p.type = 3;  // ammo shotgun
                else if (roll < 85) p.type = 4;  // ammo rifle
                else if (roll < 93) p.type = 5;  // weapon shotgun
                else p.type = 6;                  // weapon rifle

                pickups.push_back(p);
                break;
            }
        }
    }
}

void Level::PlaceLights() {
    lights.clear();
    for (int z = 0; z < MAP_SIZE; z++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            if (map[z][x] == TileType::EMPTY && GetRandomValue(0, 100) < 8) {
                LightSource l;
                l.position = {x * tileSize + tileSize * 0.5f,
                              ceilingHeight - 0.3f,
                              z * tileSize + tileSize * 0.5f};

                int colorRoll = GetRandomValue(0, 3);
                if (colorRoll == 0) l.color = {255, 200, 150, 255};
                else if (colorRoll == 1) l.color = {150, 200, 255, 255};
                else if (colorRoll == 2) l.color = {255, 150, 100, 255};
                else l.color = {200, 255, 200, 255};

                l.intensity = 0.8f + GetRandomValue(0, 40) / 100.0f;
                l.radius = 8.0f + GetRandomValue(0, 80) / 10.0f;
                l.flicker = GetRandomValue(0, 100) < 30 ? 0.1f : 0.0f;
                lights.push_back(l);
            }
        }
    }
}

void Level::BuildMeshes() {
    // We'll draw the level directly instead of building static meshes
    // for simplicity and flexibility
}

Texture2D Level::GenProceduralTexture(int w, int h, Color base, int pattern) {
    Image img = GenImageColor(w, h, base);

    for (int y = 0; y < h; y++) {
        for (int x = 0; x < w; x++) {
            int noise = GetRandomValue(-15, 15);
            Color c = base;
            c.r = (unsigned char)std::max(0, std::min(255, (int)c.r + noise));
            c.g = (unsigned char)std::max(0, std::min(255, (int)c.g + noise));
            c.b = (unsigned char)std::max(0, std::min(255, (int)c.b + noise));

            switch (pattern) {
                case 0: // Brick wall
                    if (y % 16 == 0 || ((y / 16) % 2 == 0 ? x % 32 == 0 : (x + 16) % 32 == 0)) {
                        c.r = (unsigned char)std::max(0, (int)c.r - 30);
                        c.g = (unsigned char)std::max(0, (int)c.g - 30);
                        c.b = (unsigned char)std::max(0, (int)c.b - 30);
                    }
                    break;
                case 1: // Floor tiles
                    if (x % 32 == 0 || y % 32 == 0) {
                        c.r = (unsigned char)std::max(0, (int)c.r - 20);
                        c.g = (unsigned char)std::max(0, (int)c.g - 20);
                        c.b = (unsigned char)std::max(0, (int)c.b - 20);
                    }
                    break;
                case 2: // Ceiling panels
                    if ((x % 16 < 2) || (y % 16 < 2)) {
                        c.r = (unsigned char)std::min(255, (int)c.r + 10);
                        c.g = (unsigned char)std::min(255, (int)c.g + 10);
                        c.b = (unsigned char)std::min(255, (int)c.b + 10);
                    }
                    break;
                case 3: // Door
                    if (x < 3 || x > w - 4 || y < 3 || y > h - 4) {
                        c.r = (unsigned char)std::max(0, (int)c.r - 25);
                        c.g = (unsigned char)std::max(0, (int)c.g - 25);
                        c.b = (unsigned char)std::max(0, (int)c.b - 25);
                    }
                    if (x > w/2 - 3 && x < w/2 + 3 && y > h/2 - 3 && y < h/2 + 3) {
                        c = GOLD;
                    }
                    break;
            }
            ImageDrawPixel(&img, x, y, c);
        }
    }

    Texture2D tex = LoadTextureFromImage(img);
    SetTextureFilter(tex, TEXTURE_FILTER_BILINEAR);
    SetTextureWrap(tex, TEXTURE_WRAP_REPEAT);
    UnloadImage(img);
    return tex;
}

void Level::Draw(Camera3D camera) {
    // Draw floor and ceiling for visible tiles
    for (int z = 0; z < MAP_SIZE; z++) {
        for (int x = 0; x < MAP_SIZE; x++) {
            if (map[z][x] == TileType::WALL) continue;

            float wx = x * tileSize;
            float wz = z * tileSize;

            // Distance culling
            Vector3 tileCenter = {wx + tileSize * 0.5f, 0, wz + tileSize * 0.5f};
            float dist = Vector3Distance(tileCenter, camera.position);
            if (dist > 60.0f) continue;

            // Floor
            rlSetTexture(floorTex.id);
            rlBegin(RL_QUADS);
                rlColor4ub(200, 200, 200, 255);
                rlNormal3f(0, 1, 0);
                rlTexCoord2f(0, 0); rlVertex3f(wx, 0, wz);
                rlTexCoord2f(1, 0); rlVertex3f(wx + tileSize, 0, wz);
                rlTexCoord2f(1, 1); rlVertex3f(wx + tileSize, 0, wz + tileSize);
                rlTexCoord2f(0, 1); rlVertex3f(wx, 0, wz + tileSize);
            rlEnd();
            rlSetTexture(0);

            // Ceiling
            rlSetTexture(ceilTex.id);
            rlBegin(RL_QUADS);
                rlColor4ub(180, 180, 180, 255);
                rlNormal3f(0, -1, 0);
                rlTexCoord2f(0, 0); rlVertex3f(wx, ceilingHeight, wz + tileSize);
                rlTexCoord2f(1, 0); rlVertex3f(wx + tileSize, ceilingHeight, wz + tileSize);
                rlTexCoord2f(1, 1); rlVertex3f(wx + tileSize, ceilingHeight, wz);
                rlTexCoord2f(0, 1); rlVertex3f(wx, ceilingHeight, wz);
            rlEnd();
            rlSetTexture(0);

            // Pillar
            if (map[z][x] == TileType::PILLAR) {
                float px = wx + tileSize * 0.5f;
                float pz = wz + tileSize * 0.5f;
                float pr = 0.4f;
                DrawCylinder({px, 0, pz}, pr, pr, ceilingHeight, 8,
                            {100, 90, 85, 255});
                DrawCylinderWires({px, 0, pz}, pr, pr, ceilingHeight, 8,
                                 {60, 55, 50, 255});
            }

            // Walls - check each neighbor
            // North wall (z-1)
            if (z > 0 && map[z-1][x] == TileType::WALL) {
                rlSetTexture(wallTex.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(220, 220, 220, 255);
                    rlNormal3f(0, 0, 1);
                    rlTexCoord2f(0, 1); rlVertex3f(wx, 0, wz);
                    rlTexCoord2f(1, 1); rlVertex3f(wx + tileSize, 0, wz);
                    rlTexCoord2f(1, 0); rlVertex3f(wx + tileSize, ceilingHeight, wz);
                    rlTexCoord2f(0, 0); rlVertex3f(wx, ceilingHeight, wz);
                rlEnd();
                rlSetTexture(0);
            }
            // South wall (z+1)
            if (z < MAP_SIZE - 1 && map[z+1][x] == TileType::WALL) {
                rlSetTexture(wallTex.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(200, 200, 200, 255);
                    rlNormal3f(0, 0, -1);
                    rlTexCoord2f(1, 1); rlVertex3f(wx, 0, wz + tileSize);
                    rlTexCoord2f(0, 1); rlVertex3f(wx + tileSize, 0, wz + tileSize);
                    rlTexCoord2f(0, 0); rlVertex3f(wx + tileSize, ceilingHeight, wz + tileSize);
                    rlTexCoord2f(1, 0); rlVertex3f(wx, ceilingHeight, wz + tileSize);
                rlEnd();
                rlSetTexture(0);
            }
            // West wall (x-1)
            if (x > 0 && map[z][x-1] == TileType::WALL) {
                rlSetTexture(wallTex.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(180, 180, 180, 255);
                    rlNormal3f(1, 0, 0);
                    rlTexCoord2f(0, 1); rlVertex3f(wx, 0, wz + tileSize);
                    rlTexCoord2f(1, 1); rlVertex3f(wx, 0, wz);
                    rlTexCoord2f(1, 0); rlVertex3f(wx, ceilingHeight, wz);
                    rlTexCoord2f(0, 0); rlVertex3f(wx, ceilingHeight, wz + tileSize);
                rlEnd();
                rlSetTexture(0);
            }
            // East wall (x+1)
            if (x < MAP_SIZE - 1 && map[z][x+1] == TileType::WALL) {
                rlSetTexture(wallTex.id);
                rlBegin(RL_QUADS);
                    rlColor4ub(160, 160, 160, 255);
                    rlNormal3f(-1, 0, 0);
                    rlTexCoord2f(1, 1); rlVertex3f(wx + tileSize, 0, wz + tileSize);
                    rlTexCoord2f(0, 1); rlVertex3f(wx + tileSize, 0, wz);
                    rlTexCoord2f(0, 0); rlVertex3f(wx + tileSize, ceilingHeight, wz);
                    rlTexCoord2f(1, 0); rlVertex3f(wx + tileSize, ceilingHeight, wz + tileSize);
                rlEnd();
                rlSetTexture(0);
            }
        }
    }

    // Draw light sources as glowing spheres
    for (auto& l : lights) {
        DrawSphere(l.position, 0.15f, l.color);
        // Light glow billboard would go here with shader support
    }
}

void Level::DrawPickups(float time) {
    for (auto& p : pickups) {
        if (!p.active) continue;

        float bob = sinf(time * 3.0f + p.bobOffset) * 0.15f;
        Vector3 pos = p.position;
        pos.y += bob;
        float rot = time * 90.0f + p.bobOffset * 50.0f;

        Color color;
        float size = 0.3f;
        switch (p.type) {
            case 0: color = GREEN; break;       // health
            case 1: color = BLUE; break;        // armor
            case 2: color = YELLOW; break;      // ammo pistol
            case 3: color = ORANGE; break;      // ammo shotgun
            case 4: color = YELLOW; break;      // ammo rifle
            case 5: color = RED; size = 0.4f; break;   // weapon shotgun
            case 6: color = PURPLE; size = 0.4f; break; // weapon rifle
            default: color = WHITE; break;
        }

        // Draw pickup as rotating cube
        rlPushMatrix();
        rlTranslatef(pos.x, pos.y, pos.z);
        rlRotatef(rot, 0, 1, 0);
        DrawCube({0, 0, 0}, size, size, size, color);
        DrawCubeWires({0, 0, 0}, size * 1.05f, size * 1.05f, size * 1.05f, WHITE);
        rlPopMatrix();

        // Glow effect
        DrawSphere(pos, size * 0.6f, Fade(color, 0.2f));
    }
}

void Level::Unload() {
    UnloadTexture(wallTex);
    UnloadTexture(floorTex);
    UnloadTexture(ceilTex);
    UnloadTexture(doorTex);
}

bool Level::IsWall(float x, float z) const {
    int gx = (int)(x / tileSize);
    int gz = (int)(z / tileSize);
    return IsWallTile(gx, gz);
}

bool Level::IsWallTile(int gx, int gz) const {
    if (gx < 0 || gx >= MAP_SIZE || gz < 0 || gz >= MAP_SIZE) return true;
    return map[gz][gx] == TileType::WALL || map[gz][gx] == TileType::PILLAR;
}

bool Level::CheckCollision(Vector3 pos, float radius) const {
    // Check tile at position and surrounding tiles
    int gx = (int)(pos.x / tileSize);
    int gz = (int)(pos.z / tileSize);

    for (int dz = -1; dz <= 1; dz++) {
        for (int dx = -1; dx <= 1; dx++) {
            int cx = gx + dx;
            int cz = gz + dz;
            if (!IsWallTile(cx, cz)) continue;

            // AABB vs circle collision
            float wallMinX = cx * tileSize;
            float wallMaxX = wallMinX + tileSize;
            float wallMinZ = cz * tileSize;
            float wallMaxZ = wallMinZ + tileSize;

            // For pillars, use smaller collision box
            if (cx >= 0 && cx < MAP_SIZE && cz >= 0 && cz < MAP_SIZE &&
                map[cz][cx] == TileType::PILLAR) {
                float pcx = cx * tileSize + tileSize * 0.5f;
                float pcz = cz * tileSize + tileSize * 0.5f;
                wallMinX = pcx - 0.4f;
                wallMaxX = pcx + 0.4f;
                wallMinZ = pcz - 0.4f;
                wallMaxZ = pcz + 0.4f;
            }

            float closestX = std::max(wallMinX, std::min(pos.x, wallMaxX));
            float closestZ = std::max(wallMinZ, std::min(pos.z, wallMaxZ));

            float distX = pos.x - closestX;
            float distZ = pos.z - closestZ;
            float distSq = distX * distX + distZ * distZ;

            if (distSq < radius * radius) return true;
        }
    }
    return false;
}

Vector3 Level::ResolveCollision(Vector3 oldPos, Vector3 newPos, float radius) const {
    // Try full movement
    if (!CheckCollision({newPos.x, 0, newPos.z}, radius)) return newPos;

    // Try sliding along X
    Vector3 slideX = {newPos.x, newPos.y, oldPos.z};
    if (!CheckCollision({slideX.x, 0, slideX.z}, radius)) return slideX;

    // Try sliding along Z
    Vector3 slideZ = {oldPos.x, newPos.y, newPos.z};
    if (!CheckCollision({slideZ.x, 0, slideZ.z}, radius)) return slideZ;

    // Can't move
    return {oldPos.x, newPos.y, oldPos.z};
}
