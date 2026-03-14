#pragma once
#include "raylib.h"
#include "raymath.h"
#include <vector>

enum class TileType {
    EMPTY = 0,
    WALL,
    DOOR,
    PILLAR
};

struct Pickup {
    Vector3 position;
    int type;  // 0=health, 1=armor, 2=ammo_pistol, 3=ammo_shotgun, 4=ammo_rifle, 5=weapon_shotgun, 6=weapon_rifle
    bool active = true;
    float bobOffset;
};

struct LightSource {
    Vector3 position;
    Color color;
    float intensity;
    float radius;
    float flicker;
};

struct Level {
    static const int MAP_SIZE = 32;
    TileType map[MAP_SIZE][MAP_SIZE] = {};
    float ceilingHeight = 4.0f;
    float tileSize = 4.0f;

    Vector3 spawnPoint = {0};
    Vector3 exitPoint = {0};
    bool exitUnlocked = false;

    std::vector<Pickup> pickups;
    std::vector<LightSource> lights;

    // Textures (generated procedurally)
    Texture2D wallTex;
    Texture2D floorTex;
    Texture2D ceilTex;
    Texture2D doorTex;
    Texture2D pillarTex;

    // Level mesh
    Model floorModel;
    Model ceilingModel;
    std::vector<Model> wallModels;

    int levelIndex = 0;

    void Generate(int index);
    void BuildMeshes();
    void Draw(Camera3D camera);
    void DrawPickups(float time);
    void Unload();
    bool IsWall(float x, float z) const;
    bool IsWallTile(int gx, int gz) const;
    bool CheckCollision(Vector3 pos, float radius) const;
    Vector3 ResolveCollision(Vector3 oldPos, Vector3 newPos, float radius) const;

private:
    void GenerateRooms();
    void CarveCorridor(int x1, int y1, int x2, int y2);
    void PlacePickups();
    void PlaceLights();
    Texture2D GenProceduralTexture(int w, int h, Color base, int pattern);
};
