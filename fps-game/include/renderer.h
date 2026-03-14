#pragma once
#include "raylib.h"
#include "raymath.h"

struct Level;
struct Player;

struct Renderer {
    // Post-processing
    RenderTexture2D target;
    Shader postShader;
    bool hasPostShader = false;

    // Fog settings
    Color fogColor = {10, 10, 15, 255};
    float fogDensity = 0.04f;

    // Sky
    Color skyColor = {5, 5, 10, 255};

    void Init(int width, int height);
    void BeginFrame();
    void EndFrame(int width, int height);
    void Shutdown();
    void DrawSkybox();
};
